#include "common.h"
#include "FIFOreqchannel.h"
#include <sys/wait.h>
#include <chrono>
using namespace std::chrono;
using namespace std;

string buffercapacityS;
int buffercapacity = MAX_MESSAGE;

int main(int argc, char *argv[])
{
	int opt;
	int p = 1;
	double t = 0.0;
	int e = 1;
	string filename = "";
	bool gott = false;
	bool gote = false;
	bool newChannel = false;
	int numNewChannels = 0;
	int64 filelen;
	// take all the arguments first because some of these may go to the server
	while ((opt = getopt(argc, argv, "cf:p:t:e:m:")) != -1) {
		switch (opt) {
			case 'f':
				filename = optarg;
				break;
			case 'p':
				p = stoi(optarg);
				break;
			case 't':
				t = stof(optarg);
				gott = true;
				break;
			case 'e':
				e = stoi(optarg);
				gote = true;
			case 'm':
				buffercapacityS = optarg;
				buffercapacity = stoi(optarg);
				break;
			case 'c':
				newChannel = true;
				break;
		}
	}

	int pid = fork ();
	if (pid < 0){
		EXITONERROR ("Could not create a child process for running the server");
	}
	if (!pid){ // The server runs in the child process
		char arr[buffercapacityS.length() + 1];
		strcpy(arr, buffercapacityS.c_str());
		char* args[] = {"./server", "-m", arr, nullptr};
		if (execvp(args[0], args) < 0){
			EXITONERROR ("Could not launch the server");
		}
	}
	
	FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);		//Request control channel

	auto start = high_resolution_clock::now();  	//starting timer

	if(newChannel)		//Had to duplicate everything because of the FIFORequestChannel constructor requirements
	{
		Request nc (NEWCHAN_REQ_TYPE);
		chan.cwrite (&nc, sizeof(nc));
		char nameBuf[buffercapacity];
		chan.cread(nameBuf, buffercapacity);
		FIFORequestChannel newChan(string(nameBuf), FIFORequestChannel::CLIENT_SIDE);		//Request new channel
		std::cout << "Created new channel named " << string(nameBuf) << "\n";
		if(!gott)
		{
			ofstream file1("x1.csv");
			if(!gote)
			{
				for(int i = 0; i < 1000; ++i)
				{
					DataRequest d (p, 0.004 * i, 1);
					newChan.cwrite (&d, sizeof (DataRequest)); // question
					double reply;
					newChan.cread (&reply, sizeof(double)); //answer
					file1 << 0.004 * i << "," << reply << ",";
					d = DataRequest(p, 0.004 * i, 2);
					newChan.cwrite (&d, sizeof (DataRequest)); // question
					newChan.cread (&reply, sizeof(double)); //answer
					file1 << reply << "\n";
				}
			}
			else
			{
				for(int i = 0; i < 1000; ++i)
				{
					DataRequest d (p, 0.004 * i, e);
					newChan.cwrite (&d, sizeof (DataRequest)); // question
					double reply;
					newChan.cread (&reply, sizeof(double)); //answer
					file1 << 0.004 * i << "," << reply << "\n";
				}
			}
		}
		else
		{
			DataRequest d (p, t, e);
			newChan.cwrite (&d, sizeof (DataRequest)); // question
			double reply;
			newChan.cread (&reply, sizeof(double)); //answer
			if (isValidResponse(&reply)){
				std::cout << "For person " << p <<", at time " << t << ", the value of ecg "<< e <<" is " << reply << endl;
			}
		}
		
		/* this section shows how to get the length of a file
		you have to obtain the entire file over multiple requests 
		(i.e., due to buffer space limitation) and assemble it
		such that it is identical to the original*/
		FileRequest fm (0,0);
		int len = sizeof (FileRequest) + filename.size()+1;
		char buf2 [len];
		std::memcpy (buf2, &fm, sizeof (FileRequest));
		std::strcpy (buf2 + sizeof (FileRequest), filename.c_str());
		newChan.cwrite (buf2, len);  
		newChan.cread (&filelen, sizeof(int64));
		if (isValidResponse(&filelen))
		{
			std::cout << "File length is: " << filelen << " bytes" << endl;
			ofstream file2("received/" + filename);
			len = sizeof (FileRequest) + filename.size()+1;
			for(int byteOffset = 0; byteOffset < filelen; byteOffset += buffercapacity)
			{
				int requestAmount = buffercapacity;
				if(byteOffset + buffercapacity > filelen) //If this is the last request adjust requestAmount
				{
					requestAmount = filelen - byteOffset;
				}
				char buf3[len];
				FileRequest fq(byteOffset, requestAmount);
				std::memcpy (buf3, &fq, sizeof (FileRequest));
				std::strcpy (buf3 + sizeof (FileRequest), filename.c_str());
				newChan.cwrite(buf3, len);
				char buf4[buffercapacity];
				newChan.cread(buf4, buffercapacity);	//Read data from FIFO
				file2.write(buf4, requestAmount);		//Write data to file
			}
		}
		// closing the channel    
		Request q (QUIT_REQ_TYPE);
		newChan.cwrite (&q, sizeof (Request));
	}
	else
	{
		if(!gott)
		{
			ofstream file1("x1.csv");
			if(!gote)
			{
				for(int i = 0; i < 1000; ++i)
				{
					DataRequest d (p, 0.004 * i, 1);
					chan.cwrite (&d, sizeof (DataRequest)); // question
					double reply;
					chan.cread (&reply, sizeof(double)); //answer
					file1 << 0.004 * i << "," << reply << ",";
					d = DataRequest(p, 0.004 * i, 2);
					chan.cwrite (&d, sizeof (DataRequest)); // question
					chan.cread (&reply, sizeof(double)); //answer
					file1 << reply << "\n";
				}
			}
			else
			{
				for(int i = 0; i < 1000; ++i)
				{
					DataRequest d (p, 0.004 * i, e);
					chan.cwrite (&d, sizeof (DataRequest)); // question
					double reply;
					chan.cread (&reply, sizeof(double)); //answer
					file1 << 0.004 * i << "," << reply << "\n";
				}
			}
		}
		else
		{
			DataRequest d (p, t, e);
			chan.cwrite (&d, sizeof (DataRequest)); // question
			double reply;
			chan.cread (&reply, sizeof(double)); //answer
			if (isValidResponse(&reply)){
				std::cout << "For person " << p <<", at time " << t << ", the value of ecg "<< e <<" is " << reply << endl;
			}
		}
		
		/* this section shows how to get the length of a file
		you have to obtain the entire file over multiple requests 
		(i.e., due to buffer space limitation) and assemble it
		such that it is identical to the original*/
		FileRequest fm (0,0);
		int len = sizeof (FileRequest) + filename.size()+1;
		char buf2 [len];
		std::memcpy (buf2, &fm, sizeof (FileRequest));
		std::strcpy (buf2 + sizeof (FileRequest), filename.c_str());
		chan.cwrite (buf2, len);  
		chan.cread (&filelen, sizeof(int64));
		if (isValidResponse(&filelen)){
			std::cout << "File length is: " << filelen << " bytes" << endl;
			ofstream file2("received/" + filename);
			len = sizeof (FileRequest) + filename.size()+1;
			for(int byteOffset = 0; byteOffset < filelen; byteOffset += buffercapacity)
			{
				int requestAmount = buffercapacity;
				if(byteOffset + buffercapacity > filelen)		//If this is the last request adjust requestAmount
				{
					requestAmount = filelen - byteOffset;
				}
				char buf3[len];
				FileRequest fq(byteOffset, requestAmount);
				std::memcpy (buf3, &fq, sizeof (FileRequest));
				std::strcpy (buf3 + sizeof (FileRequest), filename.c_str());
				chan.cwrite(buf3, len);						//Send request
				char buf4[buffercapacity];
				chan.cread(buf4, buffercapacity);			//Read data from FIFO
				file2.write(buf4, requestAmount);			//Write data to file
			}
		}
	}

	auto stop = high_resolution_clock::now();					//Stop clock
	auto duration = duration_cast<microseconds>(stop - start);	//Get time diff
	std::cout << "Time elapsed: " << duration.count() << "\n";
	
	// closing the channel    
	Request q (QUIT_REQ_TYPE);
	chan.cwrite (&q, sizeof (Request));
	// client waiting for the server process, which is the child, to terminate
	wait(0);
	std::cout << "Client process exited" << endl;
}
