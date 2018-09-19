#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm> 
#include <string>
#include <vector>

#define MIN_RUNTIME 60

using namespace std;

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

int main (int argc, char** argv) {
	string line;
	ifstream file;
	ofstream output;
	vector <vector <string>> dataTable;
	int CM = 0, ESB = 0, DAM = 0, jobs = 0;
	int waitTime = 0, responseTime = 0, totalWaitTime = 0, totalResponseTime = 0;
	int globalWaitTime = 0, globalResponseTime = 0;
	float slowDown = 0, boundedSlowDown = 0, totalSlowDown = 0, totalBoundedSlowDown = 0,  globalSlowDown = 0, globalBoundedSlowDown = 0;
	vector <int> waitTimeCM, waitTimeESB, waitTimeDAM;
	vector <int> responseTimeCM, responseTimeESB, responseTimeDAM;
	vector <float> slowDownCM, slowDownESB, slowDownDAM;
	vector <float> boundedSlowDownCM, boundedSlowDownESB, boundedSlowDownDAM;

	if (argc < 2) {
		printf("use: parser tracefile > output\n");
		 exit(0);
	}

	file.open(argv[1]);
	output.open(argv[2]);

	output << "JobId Partition NodeCn waitTime responseTime slowDown runTime" << endl;

	if (file.is_open()) {
		while (getline(file, line)) {
			stringstream ss(line);
			string entry, parsed;
			vector<string> row;
			int submitTime = 0, startTime = 0, endTime = 0, runTime = 0;

			jobs++;
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
			slowDown = responseTime / runTime;
			boundedSlowDown = (responseTime + max(runTime,MIN_RUNTIME)) / max(runTime,MIN_RUNTIME);
			// JobId Partition NodeCn waitTime responseTime slowDown runTime
			output << row[0] << " " << row[5] << " " << row[11] << " " << waitTime << " " << responseTime << " "  << slowDown << " " << runTime << endl;

			if (row[5] == "CM") {
				CM++;
				waitTimeCM.push_back(waitTime);
				responseTimeCM.push_back(responseTime);
				slowDownCM.push_back(slowDown);
				boundedSlowDownCM.push_back(boundedSlowDown);
			} else if (row[5] == "ESB" || row[5] == "ESB,DAM") {
				ESB++;
				waitTimeESB.push_back(waitTime);
				responseTimeESB.push_back(responseTime);
				slowDownESB.push_back(slowDown);
				boundedSlowDownESB.push_back(boundedSlowDown);
			} else if (row[5] == "DAM" || row[5] == "DAM,ESB") {
				DAM++;
				waitTimeDAM.push_back(waitTime);
				responseTimeDAM.push_back(responseTime);
				slowDownDAM.push_back(slowDown);
				boundedSlowDownDAM.push_back(boundedSlowDown);
			} else {
				cout << row[5] << endl;
			}

	        	dataTable.push_back(row);
		}
		file.close();
		output.close();
	}
	else {
		cout << "Unable to open file %s" << argv[1] << endl;
		exit(-1);
	}

	cout << "Jobs: " << jobs << endl;
	cout << "##### Partition CM - 50 nodes - " << CM << " jobs #####" << endl;
	
	totalWaitTime = 0;
	globalWaitTime = 0;
	for (int i: waitTimeCM) {
		totalWaitTime += i;
	}
	globalWaitTime += totalWaitTime;
	cout << "avgWaitTime: " << totalWaitTime / CM << endl;

	totalResponseTime = 0;
	globalResponseTime = 0;
	for (int i: responseTimeESB) {
		totalResponseTime += i;
	}
	globalResponseTime += totalResponseTime;
	cout << "avgResponseTime: " << totalResponseTime / CM << endl;

	totalSlowDown = 0;
	globalSlowDown = 0;
	for (int i: slowDownCM) {
		totalSlowDown += i;
	}
	globalSlowDown += totalSlowDown;
	cout << "avgSlowDown: " << totalSlowDown / CM << endl;

	totalBoundedSlowDown = 0;
	globalBoundedSlowDown = 0;
	for (int i: boundedSlowDownCM) {
		totalBoundedSlowDown += i;
	}
	globalBoundedSlowDown += totalBoundedSlowDown;
	cout << "avgBoundedSlowDown: " << totalSlowDown / CM << endl;


	cout << "##### Partition ESB - 150 nodes - " << ESB << " jobs #####" << endl;

	totalWaitTime = 0;
        for (int i: waitTimeESB) {
                totalWaitTime += i;
        }
	globalWaitTime += totalWaitTime;
        cout << "avgWaitTime: " << totalWaitTime / ESB << endl;

        totalResponseTime = 0;
        for (int i: responseTimeESB) {
                totalResponseTime += i;
        }
	globalResponseTime += totalResponseTime;
        cout << "avgResponseTime: " << totalResponseTime / ESB << endl;

        totalSlowDown = 0;
        for (int i: slowDownESB) {
                totalSlowDown += i;
        }
	globalSlowDown += totalSlowDown;
        cout << "avgSlowDown: " << totalSlowDown / ESB << endl;

	totalBoundedSlowDown = 0;
	for (int i: boundedSlowDownESB) {
		totalBoundedSlowDown += i;
	}
	globalBoundedSlowDown += totalBoundedSlowDown;
	cout << "avgBoundedSlowDown: " << totalSlowDown / ESB << endl;

	cout << "##### Partition DAM - 25 nodes - " << DAM << "#####" << endl;

	totalWaitTime = 0;
        for (int i: waitTimeDAM) {
                totalWaitTime += i;
        }
	globalWaitTime += totalWaitTime;
        cout << "avgWaitTime: " << totalWaitTime / DAM << endl;

        totalResponseTime = 0;
        for (int i: responseTimeDAM) {
                totalResponseTime += i;
        }
	globalResponseTime += totalResponseTime;
        cout << "avgResponseTime: " << totalResponseTime / DAM << endl;

        totalSlowDown = 0;
        for (int i: slowDownDAM) {
                totalSlowDown += i;
        }
	globalSlowDown += totalSlowDown;
        cout << "avgSlowDown: " << totalSlowDown / DAM << endl;

	totalBoundedSlowDown = 0;
	for (int i: boundedSlowDownDAM) {
		totalBoundedSlowDown += i;
	}
	globalBoundedSlowDown += totalBoundedSlowDown;
	cout << "avgBoundedSlowDown: " << totalSlowDown / DAM << endl;

	cout << "#####     #####" << endl;
	cout << "globalWaitTime: " << globalWaitTime / jobs << endl;
	cout << "globalResponseTime: " << globalResponseTime / jobs << endl;
	cout << "globalSlowDown: " << globalSlowDown / jobs << endl;

//	for (unsigned i = 0 ; i < dataTable.size(); ++i) {
//	    	cout << dataTable[i][i] << endl;
//	}
	
	exit(0);
}

