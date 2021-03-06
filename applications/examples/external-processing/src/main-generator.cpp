#include <csignal>
#include <cstring>

#include <string>
#include <thread>
#include <chrono>
#include <iostream>
#include <thread>
#include <cmath>
#include <map>

#include <ebml/IWriterHelper.h>
#include <ebml/IWriter.h>
#include <ebml/TWriterCallbackProxy.h>
#include <communication/ovCMessagingClient.h>

#include <toolkit/ovtk_defines.h>
#include <openvibe/ovIMemoryBuffer.h>
#include <openvibe/ovITimeArithmetics.h>


static bool s_DidRequestForcedQuit = false;

using namespace Communication;

static void signalHandler(int /* signal */)
{
	s_DidRequestForcedQuit = true;
}


class EBMLWriterCallback {
public:
	void write(const void* buffer, const uint64_t bufferSize)
	{
		const uint8_t* data = static_cast<const uint8_t*>(buffer);
		m_Buffer.insert(m_Buffer.end(), data, data + bufferSize);
	}

	void clear()
	{
		m_Buffer.clear();
	}

	const std::vector<uint8_t>& data()
	{
		return m_Buffer;
	}

private:
	std::vector<uint8_t> m_Buffer;
};

int main(int argc, char **argv)
{
	std::signal(SIGINT, signalHandler);

	std::string connectionID;
	unsigned int port = 49687;
	
	for (int i = 0; i < argc; i++)
	{
		if (std::strcmp(argv[i], "--connection-id") == 0)
		{
			if (argc > i + 1)
			{
				connectionID = argv[i + 1];
			}
		}
		else if (std::strcmp(argv[i], "--port") == 0)
		{
			if (argc > i + 1)
			{
				port = static_cast<unsigned int>(std::stoi(argv[i + 1]));
			}
		}
	}

	// EBML

	EBMLWriterCallback writerCallback;
	EBML::TWriterCallbackProxy1<EBMLWriterCallback> writerCallbackProxy(writerCallback, &EBMLWriterCallback::write);
	EBML::IWriter* writer = EBML::createWriter(writerCallbackProxy);
	EBML::IWriterHelper* writerHelper = EBML::createWriterHelper();
	writerHelper->connect(writer);

	s_DidRequestForcedQuit = false;

	MessagingClient client;

	client.setConnectionID(connectionID);
	
	while(!client.connect("127.0.0.1", port))
	{
		MessagingClient::ELibraryError error = client.getLastError();
		
		if (error == MessagingClient::ELibraryError::Socket_FailedToConnect)
		{
			printf("Server not responding\n");
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
		}
		else
		{
			printf("Error %d\n", error);
			exit(EXIT_FAILURE);
		}

		if (s_DidRequestForcedQuit)
		{
			exit(EXIT_SUCCESS);
		}
	}

	std::cout << "Connected to server\n";

	// Initialize

	if (client.getInputCount() > 0)
	{
		std::cerr << "The test generator can not take any inputs, was given " << client.getInputCount() << std::endl;
		client.close();
		exit(EXIT_FAILURE);
	}

	for (size_t i = 0; i < client.getOutputCount(); i++)
	{
		uint32_t index;
		uint64_t type;
		std::string name;

		if (client.getOutput(i, index, type, name))
		{
			std::cout << "Output:\n\tIndex: " << index << "\n\tType: " << type << "\n\tName: " << name << "\n\n";
		}
	}

	std::map<std::string, std::string> parameters;

	for (uint32_t i = 0 ; i < client.getParameterCount(); i++)
	{
		uint32_t index;
		uint64_t type;
		std::string name;
		std::string value;

		client.getParameter(i, index, type, name, value);
		parameters[name] = value;
	}

	if (!(parameters.count("Channel Count")))
	{
		std::cerr << "Missing parameter" << std::endl;
		client.close();
		exit(EXIT_FAILURE);
	}

	uint32_t channelCount = static_cast<uint32_t>(std::stoul(parameters.at("Channel Count")));
	uint32_t samplingRate = static_cast<uint32_t>(std::stoul(parameters.at("Sampling Rate")));
	uint32_t samplesPerBuffer = static_cast<uint32_t>(std::stoul(parameters.at("Samples Per Buffer")));
	uint32_t samplesToSend = static_cast<uint32_t>(std::stoul(parameters.at("Amount of Samples to Generate")));

	std::vector<double> matrix;
	matrix.resize(channelCount * samplesPerBuffer);

	// Announce to server that the box has finished initializing and wait for acknowledgement
	while (!client.waitForSyncMessage())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	client.pushLog(ELogLevel::LogLevel_Info, "Received Ping");

	client.pushSync();
	client.pushLog(ELogLevel::LogLevel_Info, "Sent Pong");

	// Process

	// Send the header
	writerCallback.clear();
	writerHelper->openChild(OVTK_NodeId_Header);
	{
		writerHelper->openChild(OVTK_NodeId_Header_StreamType);
		{
			writerHelper->setUIntegerAsChildData(0);
			writerHelper->closeChild();
		}
		writerHelper->openChild(OVTK_NodeId_Header_StreamVersion);
		{
			writerHelper->setUIntegerAsChildData(0);
			writerHelper->closeChild();
		}

		writerHelper->openChild(OVTK_NodeId_Header_Signal);
		{
			writerHelper->openChild(OVTK_NodeId_Header_Signal_SamplingRate);
			{
				writerHelper->setUIntegerAsChildData(samplingRate);
				writerHelper->closeChild();
			}
			writerHelper->closeChild();
		}

		writerHelper->openChild(OVTK_NodeId_Header_StreamedMatrix);
		{
			writerHelper->openChild(OVTK_NodeId_Header_StreamedMatrix_DimensionCount);
			{
				writerHelper->setUIntegerAsChildData(2);
				writerHelper->closeChild();
			}
			writerHelper->openChild(OVTK_NodeId_Header_StreamedMatrix_Dimension);
			{
				writerHelper->openChild(OVTK_NodeId_Header_StreamedMatrix_Dimension_Size);
				{
					writerHelper->setUIntegerAsChildData(channelCount);
					writerHelper->closeChild();
				}
				writerHelper->closeChild();
			}
			writerHelper->openChild(OVTK_NodeId_Header_StreamedMatrix_Dimension);
			{
				writerHelper->openChild(OVTK_NodeId_Header_StreamedMatrix_Dimension_Size);
				{
					writerHelper->setUIntegerAsChildData(samplesPerBuffer);
					writerHelper->closeChild();
				}
				writerHelper->closeChild();
			}
			writerHelper->closeChild();
		}
		writerHelper->closeChild();
	}
	if (!client.pushEBML(0, 0, 0, std::make_shared<const std::vector<uint8_t>>(writerCallback.data())))
	{
		std::cerr << "Failed to push EBML.\n";
		std::cerr << "Error " << client.getLastError() << "\n";
		exit(EXIT_FAILURE);
	}

	client.pushSync();

	uint32_t sentSamples = 0;
	while (!s_DidRequestForcedQuit || (samplesToSend != 0 && sentSamples < samplesToSend))
	{
		if (client.isEndReceived())
		{
			std::cout << "End message received!\n";
			break;
		}

		if (!client.isConnected())
		{
			std::cout << "Disconnected!\n";
			break;
		}

		if (client.isInErrorState())
		{
			std::cerr << "Error state " << client.getLastError() << "\n";
			break;
		}

		// Send data

		while (!client.waitForSyncMessage())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		uint64_t expectedSamples = OpenViBE::ITimeArithmetics::timeToSampleCount(samplingRate, client.getTime());

		while (sentSamples < expectedSamples && (samplesToSend == 0 || sentSamples < samplesToSend))
		{
			for (size_t channel = 0; channel < channelCount; channel++)
			{
				for (size_t sample = 0; sample < samplesPerBuffer; sample++)
				{
					matrix[channel * samplesPerBuffer + sample] = sin((sentSamples + sample) / static_cast<double>(samplingRate));
				}
			}

			writerCallback.clear();
			writerHelper->openChild(OVTK_NodeId_Buffer);
			{
				writerHelper->openChild(OVTK_NodeId_Buffer_StreamedMatrix);
				{
					writerHelper->openChild(OVTK_NodeId_Buffer_StreamedMatrix_RawBuffer);
					{
						writerHelper->setBinaryAsChildData(matrix.data(), matrix.size() * sizeof(double));
						writerHelper->closeChild();
					}
					writerHelper->closeChild();
				}
				writerHelper->closeChild();
			}


			uint64_t startTime = OpenViBE::ITimeArithmetics::sampleCountToTime(samplingRate, sentSamples);
			uint64_t endTime = OpenViBE::ITimeArithmetics::sampleCountToTime(samplingRate, sentSamples + samplesPerBuffer);

			if (!client.pushEBML(0, startTime, endTime, std::make_shared<const std::vector<uint8_t>>(writerCallback.data())))
			{
				std::cerr << "Failed to push EBML.\n";
				std::cerr << "Error " << client.getLastError() << "\n";
				exit(EXIT_FAILURE);
			}

			sentSamples += samplesPerBuffer;
		}

		// Errors
		uint64_t packetId;
		EError error;
		uint64_t guiltyId;

		while (client.popError(packetId, error, guiltyId))
		{
			std::cerr << "Error received:\n";
			std::cerr << "\tError: "<< static_cast<int>(error) << "\n";
			std::cerr << "\tGuilty Id: "<< guiltyId << "\n";
		}
		// Here, we send a sync message to tell to the server that we have no more data to send and we can move forward.
		if (!client.pushSync())
		{
			exit(EXIT_FAILURE);
		}
	}

	std::cout << "Processing stopped.\n";

	writerHelper->disconnect();
	writerHelper->release();
	writer->release();

	if (!client.close())
	{
		std::cerr << "Failed to close the connection\n";
	}

	return 0;
}
