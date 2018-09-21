#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm> 
#include <string>
#include <vector>

#define MIN_RUNTIME 60

using namespace std;

int CM = 0, ESB = 0, DAM = 0, jobs = 0;
int globalWaitTime = 0, globalResponseTime = 0;
double globalSlowDown = 0, globalBoundedSlowDown = 0;
vector <int> waitTimeCM, waitTimeESB, waitTimeDAM;
vector <int> responseTimeCM, responseTimeESB, responseTimeDAM;
vector <double> slowDownCM, slowDownESB, slowDownDAM;
vector <double> boundedSlowDownCM, boundedSlowDownESB, boundedSlowDownDAM;

string split(const string& str, const string& delim) {
	vector<string> tokens;
	size_t prev = 0, pos = 0;

	do {
		pos = str.find(delim, prev);
		if (pos == string::npos)
			pos = str.length();
		string token = str.substr(prev, pos-prev);
		if (!token.empty())
			tokens.push_back(token);
		prev = pos + delim.length();
	
	} while (pos < str.length() && prev < str.length());

	if (tokens.size() > 1)
		return tokens[1];
	else
		return "NULL";
}

void parseFile (ifstream *input, ofstream *output) {
	int waitTime = 0, responseTime = 0;
	double slowDown = 0, boundedSlowDown = 0;
	string line;

	*output << "JobId Partition NodeCn waitTime responseTime slowDown boundedSlowDown runTime" << endl;

	while (getline(*input, line)) {
		int submitTime = 0, startTime = 0, endTime = 0, runTime = 0;
		stringstream ss(line);
		string entry, parsed;
		vector<string> row;

		++jobs;

		while (ss >> entry) {
			parsed = split(entry,"=");
			row.push_back(parsed);
		}

		submitTime = stoi(row[7]);
		startTime = stoi(row[8]);
		endTime = stoi(row[9]);
		runTime = endTime - startTime;
		waitTime = startTime - submitTime;
		responseTime = endTime - submitTime;
		slowDown =responseTime / static_cast <double> (runTime);
		boundedSlowDown = (responseTime + max(runTime,MIN_RUNTIME)) / static_cast <double> ( max(runTime,MIN_RUNTIME));
		// JobId Partition NodeCn waitTime responseTime slowDown boundedSlowDown runTime
		*output << row[0] << " " << row[5] << " " << row[11] << " " << waitTime << " " << responseTime << " "  << slowDown << " " <<  boundedSlowDown <<  " " << runTime << endl;

		if (row[5].substr(0,2) == "CM") {
			CM++;
			waitTimeCM.push_back(waitTime);
			responseTimeCM.push_back(responseTime);
			slowDownCM.push_back(slowDown);
			boundedSlowDownCM.push_back(boundedSlowDown);
		} else if (row[5].substr(0,3) == "ESB") {
			ESB++;
			waitTimeESB.push_back(waitTime);
			responseTimeESB.push_back(responseTime);
			slowDownESB.push_back(slowDown);
			boundedSlowDownESB.push_back(boundedSlowDown);
		} else if (row[5].substr(0,3) == "DAM") {
			DAM++;
			waitTimeDAM.push_back(waitTime);
			responseTimeDAM.push_back(responseTime);
			slowDownDAM.push_back(slowDown);
			boundedSlowDownDAM.push_back(boundedSlowDown);
		} else {
			cout << row[5] << endl;
		}
	}


}

int computeTime (vector <int> accumulatedTime) {
	int total = 0;

	for (int i: accumulatedTime)
		total += i;

	return total;
}

double computeTime (vector <double> accumulatedTime) {
	double total = 0;

	for (double d: accumulatedTime)
		total += d;
		
	return total;
}

void printPartitions (string partition, int numNodes) {
	int totalWaitTime = 0, totalResponseTime = 0, numJobs = 0;
	double totalSlowDown = 0, totalBoundedSlowDown = 0;

	cout << "##### Partition " << partition << " - " << numNodes << " nodes - ";
	
	if (partition == "CM") {
		totalWaitTime = computeTime(waitTimeCM);
		totalResponseTime = computeTime(responseTimeCM);
		totalSlowDown = computeTime(slowDownCM);
		totalBoundedSlowDown = computeTime(boundedSlowDownCM);
		numJobs =  CM;
	} else if (partition == "ESB") {
		totalWaitTime = computeTime(waitTimeESB);
	        totalResponseTime = computeTime(responseTimeESB);
		totalSlowDown = computeTime(slowDownESB);
		totalBoundedSlowDown = computeTime(boundedSlowDownESB);
		numJobs = ESB;
	} else if (partition == "DAM") {
		totalWaitTime = computeTime(waitTimeDAM);
        	totalResponseTime = computeTime(responseTimeDAM);
	        totalSlowDown = computeTime(slowDownDAM);
		totalBoundedSlowDown = computeTime(boundedSlowDownDAM);
		numJobs = DAM;
	}

	cout  << numJobs << " jobs #####" << endl;

	globalWaitTime += totalWaitTime;
	cout << "avgWaitTime: " << totalWaitTime / static_cast <double> (numJobs) << endl;

	globalResponseTime += totalResponseTime;
	cout << "avgResponseTime: " << totalResponseTime / static_cast <double> (numJobs) << endl;

	globalSlowDown += totalSlowDown;
	cout << "avgSlowDown: " << totalSlowDown / numJobs << endl;

	globalBoundedSlowDown += totalBoundedSlowDown;
	cout << "avgBoundedSlowDown: " << totalBoundedSlowDown / numJobs << endl;
}

void printGlobals () {
	cout << "#####  GLOBAL VALUES   #####" << endl;
	cout << "Jobs: " << jobs << endl;
	cout << "globalWaitTime: " << globalWaitTime / static_cast <double>(jobs) << endl;
	cout << "globalResponseTime: " << globalResponseTime / static_cast <double>(jobs) << endl;
	cout << "globalSlowDown: " << globalSlowDown / static_cast <double>(jobs) << endl;
	cout << "globalBoundedSlowDown: " << globalBoundedSlowDown / static_cast <double>(jobs) << endl;
}

int main (int argc, char** argv) {
	ifstream input;
	ofstream output;

	if (argc < 2) {
		printf("use: parser tracefile > output\n");
		 exit(0);
	}

	input.open(argv[1]);
	output.open(argv[2]);

	if (input.is_open()) {
		parseFile(&input,&output);
		input.close();
		output.close();
	}
	else {
		cout << "Unable to open file %s" << argv[1] << endl;
		exit(-1);
	}

	// CM - 50 nodes
	printPartitions("CM",50); 
	printPartitions("ESB",150); 
	printPartitions("DAM",25);
	printGlobals();

	exit(0);
}

