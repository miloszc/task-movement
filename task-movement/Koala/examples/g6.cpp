#include "../io/g6.h"
#include "../io/text.h"

using namespace std;

typedef Koala::Graph<int, int> MyGraph;
typedef MyGraph::PVertex GVert;

int main() {
    MyGraph g1, g2;
    char g6format[1000];

    //we take the graph see Graph
    GVert verts[7];
    verts[0] = g1.addVert(1); verts[1] = g1.addVert(2); verts[2] = g1.addVert(3);
    verts[3] = g1.addVert(4); verts[4] = g1.addVert(5); verts[5] = g1.addVert(6);
    verts[6] = g1.addVert(7);
    g1.addEdge(verts[0], verts[2]); g1.addEdge(verts[0], verts[3]);
    g1.addEdge(verts[0], verts[5]); g1.addEdge(verts[0], verts[6]);
    g1.addEdge(verts[1], verts[2]); g1.addEdge(verts[1], verts[4]);
    g1.addEdge(verts[1], verts[5]); g1.addEdge(verts[1], verts[6]);
    g1.addEdge(verts[2], verts[3]); g1.addEdge(verts[2], verts[4]);
    g1.addEdge(verts[3], verts[4]);

    Koala::IO::writeGraphText(g1,std::cout,Koala::IO::RG_VertexLists|Koala::IO::RG_Info);

    int ans = Koala::IO::writeG6(g1, g6format, 1000);
    cout << "Write graph: " << ans << " " << g6format << "\n";
    ans = Koala::IO::readG6(g2, g6format);
    cout << "Read graph: " << ans << "\n";

    Koala::IO::writeGraphText(g2,std::cout,Koala::IO::RG_VertexLists|Koala::IO::RG_Info);

    return 0;
}

