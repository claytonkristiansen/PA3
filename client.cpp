#include "common.h"
#include "FIFOreqchannel.h"
#include <sys/wait.h>
using namespace std;

int main(int argc, char *argv[])
{
	int opt;
	int p = 1;
	double t = 0.0;
	int e = 1;
	string filename = "";
	bool gott = false;
	bool gote = false;
	buffercapacity = MAX_MESSAGE;
	// take all the arguments first because some of these may go to the server
	while ((opt = getopt(argc, argv, ":f:p:t:e:m:")) != -1) {
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
				buffercapacity = atoi(optarg);
				break;
		}
	}

	int pid = fork ();
	if (pid < 0){
		EXITONERROR ("Could not create a child process for running the server");
	}
	if (!pid){ // The server runs in the child process
		char* args[] = {"./server", nullptr};
		if (execvp(args[0], args) < 0){
			EXITONERROR ("Could not launch the server");
		}
	}
	FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);

	timeval tim;
	gettimeofday(&tim, NULL);
	suseconds_t t1 = tim.tv_usec;
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

	gettimeofday(&tim, NULL);
	suseconds_t t2 = tim.tv_usec;
	std::cout << "Time elapsed: " << t2 - t1 << "\n";
	

	
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
	int64 filelen;
	chan.cread (&filelen, sizeof(int64));
	if (isValidResponse(&filelen)){
		std::cout << "File length is: " << filelen << " bytes" << endl;
	}

	ofstream file2("received/" + filename);
	len = sizeof (FileRequest) + filename.size()+1;
	for(int byteOffset = 0; byteOffset < filelen; byteOffset += MAX_MESSAGE)
	{
		int requestAmount = MAX_MESSAGE;
		if(byteOffset + MAX_MESSAGE > filelen)
		{
			requestAmount = filelen - byteOffset;
		}
		char buf3[len];
		FileRequest fq(byteOffset, requestAmount);
		std::memcpy (buf3, &fq, sizeof (FileRequest));
		std::strcpy (buf3 + sizeof (FileRequest), filename.c_str());
		chan.cwrite(buf3, len);
		char buf4[MAX_MESSAGE];
		chan.cread(buf4, MAX_MESSAGE);
		file2.write(buf4, requestAmount);
	}
	
	
	// closing the channel    
    Request q (QUIT_REQ_TYPE);
    chan.cwrite (&q, sizeof (Request));
	// client waiting for the server process, which is the child, to terminate
	wait(0);
	std::cout << "Client process exited" << endl;

}
