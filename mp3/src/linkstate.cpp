#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <string>
#include <climits>
#include <queue>

using namespace std;
#define MAX_NODE 10
class Message{
public:
    int src;
    int dst;
    string msg;

    Message(int a, int b, const string &c){
        src =a;
        dst = b;
        msg = c;
    }
};

class Edge{
public:
    int src;
    int dst;
    int cost;
    Edge(int a, int b, int c){
        src = a;
        dst = b;
        cost = c;
    }
};

int create_topo_graph(ifstream &f, vector<vector<int> > &g, char *topo_file_path){
    int from, to, cost;
    set<int> s;
    f.open(topo_file_path);
    while(f >> from >> to >> cost){
        g[from][to] = cost;
        g[to][from] = cost;
        s.insert(from);
        s.insert(to);
    }
    f.close();
    //for(auto n : s) g[n][n] = 0;
    return s.size();
}

void read_messages(ifstream &f, vector<Message> &messages, char *msg_file_path){
    int from, to;
    string body;
    f.open(msg_file_path);
    while(f >> from >> to){
        getline(f, body);
        messages.push_back(Message(from, to, body));
    }
    f.close();
}

void read_change(ifstream &f, vector<Edge> &changes, char* change_file_path){
    f.open(change_file_path);
    int src, dst, cost;
    while(f >> src >> dst >> cost)
        changes.push_back(Edge(src, dst, cost));
    f.close();
}


void apply_change(const Edge &change, vector<vector<int> > &g){
    int from, to, cost;
    from = change.src;
    to = change.dst;
    cost = change.cost;
    if(cost == -999) cost = INT_MAX;
    g[from][to] = cost;
    g[to][from] = cost;
}

void compute_table_single(int src, int num_nodes, const vector<vector<int> > &g, vector<int> &value,
                          vector<int> &next_hop){
    //This is true Dijstra
    // comparator and priority queue
    auto cmp = [](const pair<int, int> &a, const pair<int, int> &b) -> bool {return a.second > b.second;};
    priority_queue<pair<int, int>, vector<pair<int, int> >, decltype(cmp)> Q(cmp);
    vector<int> prev(MAX_NODE + 1, -1);
    for(int i = 1; i <= num_nodes; ++i){
        value[i] = INT_MAX;
        next_hop[i] = -1;
    }
    prev[src] = -1;
    value[src] = 0;
    next_hop[src] = src;
    Q.push(pair<int, int>(src, 0));
    int cur_src, cur_cost;
    for(int i = 0; i < (MAX_NODE << 2); ++i){
        auto cur = Q.top();
        Q.pop();
        cur_src = cur.first;
        cur_cost = cur.second;
        //traverse over src's neighbours
        for(int i = 1; i <= num_nodes; ++i){
            if(g[cur_src][i] != INT_MAX){
                if(value[i] > g[cur_src][i] + cur_cost){
                    value[i] = g[cur_src][i] + cur_cost;
                    prev[i] = cur_src;
                    Q.push(pair<int, int>(i, value[i]));
                }
                else if(value[i] == g[cur_src][i] + cur_cost && prev[i] > cur_src) {
                    prev[i] = cur_src;
                }
            }
        }
        if(Q.empty()) break;
    }
    for(int i = 1; i <= num_nodes; ++i){
        if(i == src) continue;
        int st = i;
        if(value[i] < INT_MAX) {
            int cnter = 0;
            while (prev[st] != src){
                st = prev[st];
                //cout << st;
                cnter += 1;
                if(cnter >= (MAX_NODE << 2)) break;
            }
            next_hop[i] = st;
        }else next_hop[i] = -1;
    }


}

void compute_table(int num_nodes, const vector<vector<int> > &g, vector<vector<int> > &value,
                   vector<vector<int> > &next_hop){
    for(int i=1; i <= num_nodes; ++i) {
        compute_table_single(i, num_nodes, g, value[i], next_hop[i]);
    }
}

void dump_table(ofstream &f, int num_nodes, vector<vector<int> > &next_hop, vector<vector<int> > &value){
    for(int i = 1; i <= num_nodes; ++i){
        for(int j = 1; j <= num_nodes; ++j){
            if(next_hop[i][j] != -1) {
                if(value[i][j] < INT_MAX)
                    f << j << " " << next_hop[i][j] << " " << value[i][j] << "\n";
            }
        }
        f << "\n";
    }
    f << flush;
}


void send_message(ofstream &f, const vector<vector<int> > &g, const vector<vector<int> > &value,
                  const vector<vector<int> > &next_hop, const vector<Message> &messages){
    for(auto m: messages){
        if(value[m.src][m.dst] < INT_MAX) {
            vector<int> hops;
            hops.push_back(m.src);
            int st = m.src, dst = m.dst, cur;
            int cnter = 0;
            while (next_hop[st][dst] != dst) {
                cnter += 1;
                cur = next_hop[st][dst];
                hops.push_back(cur);
                st = cur;
                if(cnter >= (MAX_NODE << 2)) break;
            }
            f << "from " << m.src << " to " << m.dst << " cost " << value[m.src][m.dst] << " hops ";
            for(auto i : hops) f << i << " ";
            f << "message" << m.msg << "\n\n";

        }else
            f << "from " << m.src <<  " to " << m.dst << " cost infinite hops unreachable message" << m.msg << "\n\n";
    }

}

int main(int argc, char** argv){
    if (argc != 4)
    {
        printf("Usage: ./linkstate topofile messagefile changesfile\n");
        return -1;
    }

    vector<vector<int> > graph((MAX_NODE +1) << 1 , vector<int>((MAX_NODE +1) << 1, INT_MAX));
    //graph to traverse, initialize with big cost
    vector<vector<int> > value(MAX_NODE + 1, vector<int>((MAX_NODE+1), INT_MAX)); //record the value of graph
    vector<vector<int> > next_hop(MAX_NODE + 1, vector<int>(MAX_NODE+1, -1));
    vector<Message> msg_to_sent;
    vector<Edge> changes;
    ifstream f, f_change;
    ofstream out("output.txt");
    int num_nodes = create_topo_graph(f, graph, argv[1]);
    read_messages(f, msg_to_sent, argv[2]);
    read_change(f, changes, argv[3]);
    for(int i=0; i < changes.size()+1; ++i){
        if(i > 0) apply_change(changes[i-1], graph);
        compute_table(num_nodes, graph, value, next_hop);
        dump_table(out, num_nodes, next_hop, value);
        send_message(out, graph, value, next_hop, msg_to_sent);
    }
    out.close();
}