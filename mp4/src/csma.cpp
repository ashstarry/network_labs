#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <string>
#include <climits>
#include <queue>
#include <iomanip>

using namespace std;

int N, L, R, M, T;
vector<int> rList;
ofstream fpOut("output.txt");

class node {
private:
    int backoff;
    int collisionNumber;
    int nodeID;

public:
    node (int s) {
        this->nodeID = s;
        this->backoff = s % rList[0];
        this->collisionNumber = 0;

    }
    void setRandom(vector<int> &R, int curTime) {
        cout << "reset "<< nodeID <<"'s " << "backoff, and setTime is  = " << curTime << endl;
        this->backoff = (nodeID + curTime) % R[this->collisionNumber];
        cout << "sent success, reset backoff to = " << backoff << endl;
    }

    int getBackoff() {
        return this->backoff;
    }

    int getNodeID() {
        return this->nodeID;
    }

    int getCollisionNumber() {
        return this->collisionNumber;
    }

    void decreaseBackoff() {
        this->backoff--;
    }

    void increaseCollisionNumber() {
        this->collisionNumber++;
    }

    void resetCollisionNumber() {
        this->collisionNumber = 0;
    }
};

void simulate(int nodeNumber) {

    vector<node*> nodeArray(nodeNumber);
    // initialization
    for (int i = 0; i < nodeNumber; i++) {
        nodeArray[i] = new node(i);
//        cout << "currentNode Backoff = " << nodeArray[i]->backoff << endl;
//        cout << "currentNode NodeId = " << nodeArray[i]->nodeID << endl;
    }
    // variables for statistics
    int curOccupyNode = -1;
    int packetSentNumber = 0;
    int endTime = 0;
    // int channelOccupied = 0;
    // container for current nodes ready to be sent
    vector<int> nodeSent;

    // real time simulator
    for (int time = 0; time < T; time++) {
        cout << "curTIme in main loop = " << time << endl;
        nodeSent.clear();
        for (int i = 0; i  < nodeArray.size(); i++) {
            if (nodeArray[i]->getBackoff() == 0) {
                nodeSent.push_back(i);
                cout << "node_index = " << nodeArray[i]->getNodeID() << endl;
            }
        }
        int sendNumber = nodeSent.size();
        cout << "sendNum = " << sendNumber << endl;

        if (sendNumber == 0) {
            // all idle
            for (int i = 0; i < nodeNumber; i++) {
                nodeArray[i]->decreaseBackoff();
            }
        }
        // sendNumber = 1: successfully transmit
        else if (sendNumber == 1) {
            curOccupyNode = nodeSent[0];
            cout << "curOccupyNode = " << curOccupyNode << endl;
            if (time + L > T) {
                endTime = T - time;
                break;
            }

            time += L-1;
            packetSentNumber++;

            // reset
            nodeArray[curOccupyNode]->resetCollisionNumber();
            nodeArray[curOccupyNode]->setRandom(rList,time + 1);

            // need focus here
            curOccupyNode = -1;
            cout << "reset_curOccupyNode" << endl;
        }
            // sendNumber > 1: collision capture
        else {
            for (int i = 0; i < sendNumber; i++) {
                nodeArray[nodeSent[i]]->increaseCollisionNumber();
                if (nodeArray[nodeSent[i]]->getCollisionNumber() == M) {
                    nodeArray[nodeSent[i]]->resetCollisionNumber();
                }
                nodeArray[nodeSent[i]]->setRandom(rList,time + 1);
            }

        }

        cout << "packetSentNum = " << packetSentNumber << endl;
        cout << "endtime = " << endTime << endl;
    }
    cout << "final_packetSentNum = " << packetSentNumber << endl;
    cout << "final_endtime = " << endTime << endl;
    double result = (double) (packetSentNumber * L + endTime) / T;
    ostringstream o1;
    o1.setf(ios::fixed);
    o1 << setprecision(2) << result;
    string towrite = o1.str();
//    cout << towrite << endl;
//    fpOut << nodeNUm <<" "<<(packetSentNum * L + endtime) * 1.0 / T << endl;
    fpOut << towrite << endl;
}
int main(int argc, char **argv) {

    if (argc != 2) {
        printf("Usage: ./csma input.txt\n");
        return -1;
    }

    ifstream in;
    in.open(argv[1]);
    char c;
    in >> c >> N >> c >> L >> c >> M >> c;
//    cout << "nodeNum = " << N << endl;
//    cout << "dataLen = " << L << endl;

    while (in.get() != '\n') {
        in >> R;
        rList.push_back(R);
//        cout << "random range = " << R << endl;
    }
    in >> c >> T;
//    cout << "Max_retransmit = " << M << endl;
//    cout << "Tick = " << T << endl;


    simulate(N);
    fpOut.close();

    return 0;
}



