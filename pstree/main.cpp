#include <getopt.h>
#include <assert.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include <vector>
#include <unordered_map>
#include <set>
#include <algorithm>

#include <filesystem>

namespace fs = std::filesystem;

typedef std::pair<int, std::string> data;

typedef struct Node {
	data d;
	std::vector<data> threads;
	std::vector<data> processes;
} Node;

struct Tree {
	std::unordered_map<int, Node*> pidMap;
	std::set<data> pidNamePair;
} Tree;

static struct option long_options[] = {
	{"show-pids"    , no_argument , NULL , 'p' } ,
	{"numeric-sort" , no_argument , NULL , 'n' } ,
	{"version"      , no_argument , NULL , 'V' } ,
	{NULL           , 0           , NULL , 0   } ,
};

static struct {
	int startPID = 1;

	int isVerbose = 0;
	int isShowPID = 0;
	int isSortByPID = 0;
} config;

/// @brief Parse input args
/// @param argc number of arguments
/// @param argv arguments lists
/// @return 0 if parse success, 1 if parse fail
int parseArgs(int argc, char** argv) {
	int opt, longindex;

	while (( opt = getopt_long(argc, argv, "pnV", long_options, &longindex) ) != -1) {
		switch (opt) {
		case 'p':
			config.isShowPID = 1; break;
		case 'n':
			config.isSortByPID = 1; break;
		case 'V':
			config.isVerbose = 1; break;
		default:
			return 1;
		}
	}

	if (!(optind == argc || optind == argc - 1)) {
		return 1;
	}

	if (optind == argc - 1) {
		config.startPID = atoi(argv[optind]);
	}

	return 0;
}

std::string readFromFile(std::string inFileStr) {
    std::ifstream fin(inFileStr);
    std::ostringstream sout;
    std::copy(std::istreambuf_iterator<char>(fin),
            	std::istreambuf_iterator<char>(),
            	std::ostreambuf_iterator<char>(sout));

    return sout.str();
}

std::string getStatusName(fs::path &statusFilePath) {
	assert(fs::exists(statusFilePath) && fs::is_regular_file(statusFilePath));
	auto contents = readFromFile(statusFilePath.string());
	assert(std::equal(contents.begin(), contents.begin() + 5, "Name:"));
	auto lastChar = contents.find_first_of('\n');
	auto firstChar = contents.find_first_not_of(' ', 6);
	auto processName = std::string(contents.begin() + firstChar, contents.begin() + lastChar);

	return processName;
}

std::vector<int> getChildrenPID(fs::path &childrenFilePath) {
	std::vector<int> childrenPID;

	assert(fs::exists(childrenFilePath) && fs::is_regular_file(childrenFilePath));
	auto contents = readFromFile(childrenFilePath.string());
	auto endBlank = contents.find_last_of(' ');
	while (endBlank != std::string::npos) {
		auto firstBlank = contents.find_first_of(' ');
		int pid = atoi(contents.substr(0, firstBlank).c_str());
		childrenPID.push_back(pid);

		contents = contents.substr(firstBlank + 1, endBlank - firstBlank + 1);
		endBlank = contents.find_last_of(' ');
	}

	return childrenPID;
}

std::string getNameFromPID(int pid) {
	std::string name = "";
	for (auto p : Tree.pidNamePair) {
		if (p.first == pid) {
			name = p.second;
			break;
		}
	}
	return name;
}

/// @brief 
/// @param startPID 
/// @return 0 for build Tree success, 1 for build Tree fail
int buildTreeFromPID(int startPID) {
	Node *n = new Node;

	std::string processPathStr = "/proc/" + std::to_string(startPID);
	fs::path processPath = processPathStr;
	if (!fs::exists(processPath) || !fs::is_directory(processPath)) {
		std::cerr << processPath << " not exists or is not a directory" << std::endl;
		return 1;
	}

	fs::path statusFilePath = processPath / "status";
	auto processName = getStatusName(statusFilePath);

	Tree.pidNamePair.insert({startPID, processName});
	Tree.pidMap[startPID] = n;
	n->d = {startPID, processName};

	auto taskPath = processPath / "task";
	assert(fs::exists(taskPath) && fs::is_directory(taskPath));

	std::vector<int> tids;
	for (auto &ii : fs::directory_iterator(taskPath)) {
		fs::path p = ii.path();
		assert(fs::exists(p) && fs::is_directory(p));
		tids.push_back(atoi(p.filename().string().c_str()));
	}

	for (auto tid : tids) {
		auto tidPath = taskPath / std::to_string(tid);
		assert(fs::exists(tidPath) && fs::is_directory(tidPath));

		if (!(tid == startPID)) {
			fs::path statusFilePath = tidPath / "status";
			auto tidName = getStatusName(statusFilePath);
			n->threads.push_back({tid, tidName});
		}

		fs::path childrenFilePath = tidPath / "children";
		std::vector<int> childrenPID = getChildrenPID(childrenFilePath);
		for (auto childPID : childrenPID) {
			buildTreeFromPID(childPID);
			std::string childName = getNameFromPID(childPID);
			assert(!childName.empty());
			n->processes.push_back({childPID, childName});
		}
	}

	// std::cerr << processName << std::endl;
	return 0;
}

void printTree(int startPID, int indent=0) {
	auto f_makeIndent = [](int indent) {
		std::string perIndent = "  ", retval = "";
		for (int i = 0; i < indent; ++i) {
			retval += perIndent;
		}
		return retval;
	};
	std::string indentStr = f_makeIndent(indent),
				indentPlusStr = f_makeIndent(indent + 1);
	std::string value = getNameFromPID(startPID);
	if (config.isShowPID) {
		value += "(" + std::to_string(startPID) + ")";
	}
	std::cout << indentStr << value << std::endl;

	Node *n = Tree.pidMap[startPID];
	for (auto t : n->threads) {
		std::string tvalue = t.second;
		if (config.isShowPID) {
			tvalue += "("  + std::to_string(t.first) + ")";
		}
		tvalue = "{" + tvalue + "}";
		std::cout << indentPlusStr << tvalue << std::endl;
	}

	for (auto p : n->processes) {
		printTree(p.first, indent + 1);
	}
}

std::string prettyPrintTree(int startPID, std::string &firstLine, std::string secondLine) {
	std::string value = getNameFromPID(startPID);
	if (config.isShowPID) {
		value += "(" + std::to_string(startPID) + ")";
	}

	auto f_makeIndent = [](int indent) {
		std::string perIndent = " ", retval = "";
		for (int i = 0; i < indent; ++i) {
			retval += perIndent;
		}
		return retval;
	};

	Node *n = Tree.pidMap[startPID];
	int processNum = n->processes.size();
	int threadNum = n->threads.size();
	int totalPrintNum = processNum + threadNum;

	int valueSize = value.size();
	firstLine += value;
	if (totalPrintNum != 0) {
 		firstLine += "-+-";
	}
	secondLine += f_makeIndent(valueSize + 1) + "|" + " ";
	std::string secondConnectLine = secondLine;
	std::string lastLine = secondLine;
	std::string lastSecondLine = secondLine;
	secondConnectLine.replace(secondConnectLine.size() - 1, 1, "-");
	lastLine.replace(lastLine.size() - 2, 2, "`-");
	lastSecondLine.replace(lastLine.size() - 2, 1, " ");

	if (config.isSortByPID) {
		struct {
			bool operator() (data a, data b) {
				return a.first < b.first;
			}
		} pidCompareOp;
		std::sort(n->threads.begin(), n->threads.end(), pidCompareOp);
		std::sort(n->processes.begin(), n->processes.end(), pidCompareOp);
	}

	std::string returnContents;
	for (int i = 0; i < totalPrintNum; ++i) {
		if (i < processNum) {
			// Print Process
			int targetPID = n->processes[i].first;
			if (i == 0) {
				std::string contents = prettyPrintTree(targetPID, firstLine, secondLine);
				returnContents += contents;
			} else if (i == totalPrintNum - 1) {
				std::string lastLine_c = lastLine;
				std::string contents = prettyPrintTree(targetPID, lastLine_c, lastSecondLine);
				returnContents += lastLine_c + "\n" + contents;
			} else {
				std::string secondLine_c = secondConnectLine;
				std::string contents = prettyPrintTree(targetPID, secondLine_c, secondLine);
				returnContents += secondLine_c + "\n" + contents;
			}
		} else {
			// Print Threads
			int targetTID = n->threads[i - processNum].first;
			std::string targetName = n->threads[i - processNum].second;
			if (config.isShowPID) {
				targetName += "(" + std::to_string(targetTID) + ")";
			}
			targetName = "{" + targetName + "}";
			if (i == 0) {
				firstLine += targetName;
			} else if (i == totalPrintNum - 1) {
				returnContents += lastLine + targetName + "\n";
			} else {
				returnContents += secondConnectLine + targetName + "\n";
			}
		}
	}
	return returnContents;
}

int main(int argc, char** argv) {
	if (parseArgs(argc, argv)) {
		printf("Usage: %s [-n, --numeric-sort] [-p, --show-pids] [pid]\n"
			   "       %s -V, --version\n", argv[0], argv[0]);
		return 1;
	}

	buildTreeFromPID(config.startPID);

	// printTree(config.startPID);
	std::string firstLine;
	std::string output = prettyPrintTree(config.startPID, firstLine, "");
	std::cout << firstLine << std::endl << output;

	return 0;
}
