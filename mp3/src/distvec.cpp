#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <climits>

#include <stack>
#define MAX_NODE 10
using namespace std;

// topoTable
int nodeNum = MAX_NODE;
int topoTable[MAX_NODE + 1][MAX_NODE + 1];

// save nodes
set<int> nodes;

// msg2Sent struct
typedef struct msg2Sent{
    int src_node;
    int dst_node;
    string msgContent;
    msg2Sent(int src_node, int dst_node, string msgContent): src_node(src_node), dst_node(dst_node), msgContent(msgContent){

    }
}msg2Sent;
//list for msg2Sent
vector<msg2Sent> msgs;

// output file
ofstream fpOut("output.txt");

// define forwardTable
// each node has a forwardTable
// tempTable for pre_node
typedef map<int, pair<int, int> > forwardTable;
// dst_node, pre_node, cost
forwardTable tempTable[11];
// dst_node, next_node, cost
forwardTable final_forwardTable[11];

void extractMsg(ifstream &msgFile)
{
    int src_node, dst_node;

//     messagefile(argv[2]);

    string line, msgContent;
    int timer = 0;
    while(getline(msgFile, line))
    {
        if(line != "")
        {
            stringstream line_ss(line);
            line_ss >> src_node >> dst_node;
            getline(line_ss, msgContent);
            msg2Sent msg(src_node, dst_node, msgContent.substr(1));
            msgs.push_back(msg);
            timer++;
            if(timer == 10){
                break;
            }
        }
    }
}


void extractTopo(ifstream &topofile)
{
    int src_node, dst_node, cost;

    for(int i=1; i<=nodeNum; i++)
    {
        for(int k=1; k<=nodeNum; k++)
        {
            topoTable[i][k] = -999;
            if(i==k)
            {
                topoTable[i][k] = 0;
            }
        }
    }

    // initialize graph
    while(topofile >> src_node >> dst_node >> cost)
    {
        topoTable[src_node][dst_node] = cost;
        topoTable[dst_node][src_node] = cost;
        nodes.insert(src_node);
        nodes.insert(dst_node);
    }
}

void tempTable_init()
{
    int src_node, dst_node;

    for(set<int>::iterator i=nodes.begin(); i!=nodes.end(); i++)
    {
        src_node = *i;
        for(set<int>::iterator k=nodes.begin(); k!=nodes.end(); k++)
        {
            dst_node = *k;
//            write each node's relationship with every other nodes
            tempTable[src_node][dst_node] = make_pair(src_node, topoTable[src_node][dst_node]);

        }
    }

//    cout << "--------" << endl;
//    for(int i=0; i<11; i++){
//        forwardTable curr = tempTable[i];
//        forwardTable::iterator it;
//        //第一种
//        for(it = curr.begin(); it != curr.end(); ++it){
//            cout<<it->first<< " " << it->second.first << " " << it->second.second << endl;
//        }
//    }
//    cout << "--------" << endl;
}



void forwardTable_create()
{
    int nodesNum = nodes.size();
    int distance[nodesNum + 1];
    int next_node[nodesNum + 1];
    int start_node;

    //iterate each node as start node
    for (set<int>::iterator it = nodes.begin(); it != nodes.end(); it++) {
        start_node = *it;
        //cout << start_node << endl;
        //initialize distance array with infinite distance
        for (int i=1; i<=nodesNum; i++) {
            distance[i] = 999;
            next_node[i] = -1;
        }
        distance[start_node] = 0;

        //loose edges
        for (int i=0; i<nodesNum-1; i++) {
            //get valuable edges
            for (int j=1; j<=nodesNum; j++) {
                for (int k=1; k<=nodesNum; k++) {
                    //j from k to topo[i][j] distance
                    if (k != start_node && topoTable[j][k] != 0 && topoTable[j][k] != -999){
                        int weight = topoTable[j][k];
                        if (distance[j] != 999 && distance[k] > distance[j] + weight) {
                            //if (distance[k] > distance[j] + weight) {
                            distance[k] = distance[j] + weight;
                            next_node[k] = j;
                            //check = 1; //数组distance发生更新，改变check的值
                        }
                    }
                }
            }
        }
        //cout << "-------" << endl;
        //cout << start_node << endl;
//        for (int i = 1; i <= nodesNum; i++) {
//            //i destination node
//            //fpOut<<i<<" "<<final_forwardTable[src_node][dst_node].first<<" "<<final_forwardTable[src_node][dst_node].second<<endl;
//            printf("%d ", distance[i]);
//        }
//        cout << "\n-------" << endl;
        for (int i = 1; i <= nodesNum; i++) {
            //i destination node
            int dst_node = i;
            int next_hop = -1;
            //fpOut<<i<<" "<<final_forwardTable[src_node][dst_node].first<<" "<<final_forwardTable[src_node][dst_node].second<<endl;
            //unreachable
            if(distance[i] == 999){
                final_forwardTable[start_node][dst_node].first = next_hop;
                final_forwardTable[start_node][dst_node].second = -distance[i];
                //cout << endl;
                fpOut<<dst_node<<" "<<final_forwardTable[start_node][dst_node].first<<" "<<final_forwardTable[start_node][dst_node].second<<endl;
                //cout << dst_node << " " <<final_forwardTable[start_node][dst_node].first<<" "<<final_forwardTable[start_node][dst_node].second<<endl;
                continue;
            }
            stack<int> stackVertices;
            int k = dst_node;
            do{
                stackVertices.push(k);
                k = next_node[k];
            } while (k != next_node[k] && k != -1);
            if(start_node == dst_node)
                next_hop = stackVertices.top();
            //cout << stackVertices.top();
            stackVertices.pop();

//            cout << stackVertices.top();
//            while(!stackVertices.empty())
//                stackVertices.pop();
            unsigned int nLength = stackVertices.size();
            for (unsigned int nIndex = 0; nIndex < nLength; nIndex++)
            {
                if(next_hop == -1){
                    next_hop = stackVertices.top();
                }
                //cout << " -> " << stackVertices.top();
                stackVertices.pop();
            }
            //cout << endl;
            final_forwardTable[start_node][dst_node].first = next_hop;
            final_forwardTable[start_node][dst_node].second = distance[i];
            //cout << endl;
            fpOut<<dst_node<<" "<<final_forwardTable[start_node][dst_node].first<<" "<<final_forwardTable[start_node][dst_node].second<<endl;
            //cout << dst_node << " " <<final_forwardTable[start_node][dst_node].first<<" "<<final_forwardTable[start_node][dst_node].second<<endl;

        }
        //cout << "\n-------" << endl;
    }
    //fpOut<<endl;
//    cout << "--------" << endl;
//    for(int i=0; i<11; i++){
//        forwardTable curr = final_forwardTable[i];
//        forwardTable::iterator it;
//        for(it = curr.begin(); it != curr.end(); ++it){
//            cout<<it->first<< " " << it->second.first << " " << it->second.second << endl;
//        }
//    }
//    cout << "--------" << endl;
}


void msgTransfer()
{
    int src_node, dst_node, temp_id;
    for(int i=0; i<msgs.size(); i++)
    {
        src_node = msgs[i].src_node;
        dst_node = msgs[i].dst_node;
        temp_id = src_node;
        //cout << src_node << endl;
        //cout << dst_node << endl;
        fpOut<<"from "<<src_node<<" to "<<dst_node<<" cost ";
        //cout << "test3" << endl;
        if(final_forwardTable[src_node][dst_node].second<0)
        {
            fpOut<<"infinite hops unreachable ";
        }
        else if(final_forwardTable[src_node][dst_node].second==0)
        {
            fpOut<<final_forwardTable[src_node][dst_node].second<<" hops ";
        }
        else
        {
            fpOut<<final_forwardTable[src_node][dst_node].second<<" hops ";
            while(temp_id!=dst_node)
            {
                fpOut<<temp_id<<" ";
                temp_id = final_forwardTable[temp_id][dst_node].first;
            }
        }
        fpOut<<"message "<<msgs[i].msgContent<<endl;
    }
    //fpOut<<endl;
}



int main(int argc, char** argv)
{

    if (argc != 4)
    {
        printf("Usage: ./linkstate topofile messagefile changesfile\n");
        return -1;
    }

    // extract messages
    ifstream msgFile(argv[2]);
    extractMsg(msgFile);

    // extract data
    ifstream topofile(argv[1]);
    extractTopo(topofile);

    // initialization
    tempTable_init();

    // Dijkstra
    forwardTable_create();

    // transfer messages
    msgTransfer();
    //cout << "test2" << endl;
    int src_node, dst_node, cost;

    // change
    ifstream changesfile(argv[3]);
    int timer = 10;
    while(changesfile >> src_node >> dst_node >> cost)
    {
        topoTable[src_node][dst_node] = cost;
        topoTable[dst_node][src_node] = cost;
        tempTable_init();
        forwardTable_create();
        msgTransfer();
        timer++;
        if (timer == 10)
            break;
    }

    fpOut.close();

/*
	if (argc != 4) {
        printf("Usage: ./distvec topofile messagefile changesfile\n");
        return -1;
    }

    FILE *fpOut;
    fpOut = fopen("output.txt", "w");
    fclose(fpOut);
*/
    return 0;
}



