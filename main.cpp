#include <algorithm>
#include <assert.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <random>
#include <stdio.h>
#include <stdlib.h>
#include <unordered_set>
#include <vector>
#include "xoshiro256plusplus.h"
#include "xoshiro256plusplus.c"
#define RANDDEV "/dev/urandom"  // This is not a high-security application, so urandom is perfectly good.

// std::random_device gets a random number from OS, is slow. Use it to get the seed.
// A generator with a longer period is not really important as long as it has P>t^2 where
// n is the number of values you need (https://prng.di.unimi.it/). Need a generator that is
// very uniform. Apparently xoshiro256++/xoshiro256** are both good.

/*### Randomization ###*/
uint64_t getSeed() {
    // Generate a random uint64_t from Linux /dev/urandom.
    // If the OS is not Linux, this will need to be replaced.
    // Might also work on MacOS.
    uint64_t seed;
    FILE *rdp;
    rdp = fopen(RANDDEV, "rb");
    assert(rdp);
    assert(fread(&seed, sizeof(seed), 1, rdp) == 1);
    fclose(rdp);
    return (seed);
}


void set_seed() {
    for (int i =0; i < 4; i++) {s[i] = getSeed();}
}


uint64_t next_int(int i) {
    return next() % i;
}


/* ### Set Operations ### */
std::unordered_set <int> setIntersect(std::unordered_set <int>& a, std::unordered_set <int>& b) {
    std::unordered_set <int> aIb = std::unordered_set<int>();
    std::unordered_set<int>::iterator b_iter;
    for (b_iter = b.begin(); b_iter != b.end(); ++b_iter) {
        if(a.count(*b_iter)) {
            aIb.insert(*b_iter);
        }
    }
    return aIb;
}

std::unordered_set <int> setUnion(std::unordered_set <int>& a, std::unordered_set <int>& b) {
    std::unordered_set <int> aUb = a;  // this *should* copy a, need to watch to see if that's what actually happens.
    std::unordered_set<int>::iterator b_iter;
    for (b_iter = b.begin(); b_iter != b.end(); ++b_iter) {
            aUb.insert(*b_iter);
    }
    return aUb;
}


/* ### Graph Stuff ### */
// Make an object to handle graph randomization?
// Said object could consist of sets representing rows or columns.
// Can give it a method to grab two random rows.
// Note: For a dense matrix, the benefit of using sets over using a
// simple adjacancy matrix are small if any. Maybe we can try to decide
// ahead of time which to use. For now, let's just use an array of pointers to sets,
// then make an adjacency matrix implementation when the time comes.
class HashGraph {
public:
    HashGraph(std::unordered_set <int>*adjArr, int nRows, int nCols, bool transposed)
        : m_adjArr(adjArr), m_nRows(nRows), m_nCols(nCols), transposed(transposed) {}
    void swap();
    void swap_test();
    void swap_n_times(int swaps);
    HashGraph* copy();
    std::vector<int> edgelist();
    // TODO: For these functions, do we need to account for whether the matrix is transposed?
    int nRows() {return m_nRows;}
    int nCols() {return m_nCols;}
    bool is_transposed() {return transposed;}
    ~HashGraph(){  // The destructor
        delete[] m_adjArr;  delete[] ab_symmetric_difference;
        delete aIbSet; delete aUbSet;
    }

protected:
    private:
        std::unordered_set <int>* m_adjArr {NULL};
        int m_nRows {0};
        int m_nCols {0};
        bool transposed {false};  // This will remind us to transpose again to get a correct graph.
        // These are like buffers
        std::unordered_set <int> *aIbSet = new std::unordered_set<int>();
        std::unordered_set <int> *aUbSet = new std::unordered_set<int>();
        // This is like a buffer that doesn't need to be reallocated each time shuffle is ran.
        int* ab_symmetric_difference = new int[m_nCols];


        void setIntersect(std::unordered_set <int>& a, std::unordered_set <int>& b);
        void setUnion(std::unordered_set <int>& a, std::unordered_set <int>& b);
        void shuffle(int row1, int row2);
};


/* ### Set Operations ### */
void HashGraph::setIntersect(std::unordered_set <int>& a, std::unordered_set <int>& b) {
    std::unordered_set<int>::iterator b_iter;
    aIbSet->clear();
    for (b_iter = b.begin(); b_iter != b.end(); ++b_iter) {
        if(a.count(*b_iter)) {
            aIbSet->insert(*b_iter);
        }
    }
}


void HashGraph::setUnion(std::unordered_set <int>& a, std::unordered_set <int>& b) {
    aUbSet->clear();
    std::unordered_set<int>::iterator b_iter;
    for (b_iter = a.begin(); b_iter != a.end(); ++b_iter) {
            aUbSet->insert(*b_iter);
    }
    for (b_iter = b.begin(); b_iter != b.end(); ++b_iter) {
            aUbSet->insert(*b_iter);
    }
}


HashGraph* HashGraph::copy(){
    std::unordered_set <int>* adjArr = { new std::unordered_set<int>[nRows()] };
    for (int i = 0; i < nRows(); i++){
        adjArr[i] = m_adjArr[i];  // This should copy the data over, right?
    }
    HashGraph* clone = new HashGraph(adjArr, nRows(), nCols(), transposed);
    return clone;
}



void HashGraph::shuffle(int row1, int row2) {
    std::unordered_set <int> *row1Set = &m_adjArr[row1];
    std::unordered_set <int> *row2Set = &m_adjArr[row2];

    // Find the intersection and union
    setIntersect(*row1Set, *row2Set);
    setUnion(*row1Set, *row2Set);
    int row1Set_length = row1Set->size();

    // Make sure they both have swappable elements for this step. Otherwise there's no point.
    if (aIbSet->size() == row1Set_length || aIbSet->size() == row2Set->size()) {
        return;  // No swap is made.
    }
    std::unordered_set<int>::iterator aUb_iter = aUbSet->begin();
    int ab_sym_difference_size = aUbSet->size() - aIbSet->size();
    //int* ab_symmetric_difference = new int[ab_sym_difference_size];
    for (int i = 0; i < ab_sym_difference_size; i++) {
        while (aIbSet->count(*aUb_iter)){
            ++aUb_iter;
        }
        ab_symmetric_difference[i] = *aUb_iter;
        // While we're creating av_symmetric_difference, we may as well do the next step.
        row1Set->erase(*aUb_iter);
        row2Set->erase(*aUb_iter);
        ++aUb_iter;
    }
    // Allocate the exchangeable elements:
    // First shuffle them randomly.
    std::random_shuffle(ab_symmetric_difference, ab_symmetric_difference + ab_sym_difference_size, next_int);
    // Now add the appropriate number of elements to each set.
    int row1Exclusive = row1Set_length - aIbSet->size();
    for (int i = 0; i < row1Exclusive; i++) {
        row1Set->insert(ab_symmetric_difference[i]);
    }
    for (int i = row1Exclusive; i < ab_sym_difference_size; i++) {
        row2Set->insert(ab_symmetric_difference[i]);
    }
    aIbSet->clear();
    aUbSet->clear();
    //delete[] ab_symmetric_difference;
}


void HashGraph::swap() {
    // This is where the edges will be traded between two vertices in a random fashion.

    // Select two rows at random
    int row1 = next() % nRows();
    int row2 = next() % (nRows() - 1);
    if (row2 == row1) row2 ++;
    shuffle(row1, row2);
}


void HashGraph::swap_n_times(int swaps) {
    for (int i = 0; i < swaps; i++) {
        swap();
    }
}



// This is a terse, but kind of ugly, way of doing this; it only has to be done once though.
template<int R, int C>
HashGraph makeHashGraph(bool (&adjArr)[R][C]) {
    bool transpose = false;
    int nRows = R, nCols = C;
    if (nRows > nCols) {
        int third = nRows;
        nRows = nCols;
        nCols = third;
        transpose = true;
    }
    std::unordered_set <int>* m_adjArr = { new std::unordered_set<int>[nRows] };
    if (transpose) {
        for (int i = 0; i < nRows; i++) {
            for (int j = 0; j < nCols; j++) {
                if (adjArr[j][i] == 1) m_adjArr[i].insert(j);
            }
        }
    }
    else {
        for (int i = 0; i < nRows; i++) {
            for (int j = 0; j < nCols; j++) {
                if (adjArr[i][j] == 1) m_adjArr[i].insert(j);
            }
        }
    }
    return HashGraph(m_adjArr, nRows, nCols, transpose);
}


HashGraph makeHashGraph(std::vector<std::vector<bool>> adjArr, int nRows, int nCols) {
    bool transpose = false;
    if (nRows > nCols) {
        int third = nRows;
        nRows = nCols;
        nCols = third;
        transpose = true;
    }
    std::unordered_set <int>* m_adjArr = new std::unordered_set <int>[nRows];
    if (transpose) {
        puts("Transposed");
        for (int i = 0; i < nRows; i++) {
            for (int j = 0; j < nCols; j++) {
                if (adjArr[j][i] == 1) m_adjArr[i].insert(j);
            }
        }
    }
    else {
        for (int i = 0; i < nRows; i++) {
            for (int j = 0; j < nCols; j++) {
                if (adjArr[i][j] == 1) m_adjArr[i].insert(j);
            }
        }
    }
    return HashGraph(m_adjArr, nRows, nCols, transpose);
}


/* ### IO ### */
HashGraph read_edgelist(char filepath[]) {
    // TODO: This may be a bit wasteful. Could we try to make a list of the vertices that have nonzero degree?
    int imax = 0, jmax = 0;
    FILE * edgelist_file = fopen(filepath, "r");
    char line_buffer[100];  // Should be more than enough.
    if (edgelist_file == NULL) {
        perror("Error opening file");
    }
    while (! feof (edgelist_file)) {
        if (fgets(line_buffer, 100, edgelist_file) == NULL) break;
        int i, j;
        sscanf(strtok(line_buffer, "\t"), "%d", &i);
        sscanf(strtok(NULL, "\n"), "%d", &j);
        imax = std::max(imax, i);
        jmax = std::max(jmax, j);
    }
    fclose(edgelist_file);
    std::vector<std::vector<bool>> adj_matrix = std::vector<std::vector<bool>>(imax + 1, std::vector<bool>(jmax + 1));
    edgelist_file = fopen(filepath, "r");
    if (edgelist_file == NULL) {
        perror("Error opening file");
    } else {
        while (! feof (edgelist_file)) {
            if (fgets(line_buffer, 100, edgelist_file) == NULL) break; // read to newline into buffer
            //fputs(line_buffer, stdout);  // this is from the example, should print to stdout.
            int i = 0, j = 0;
            sscanf(strtok(line_buffer, "\t "), "%d", &i);
            sscanf(strtok(NULL, "\n\t "), "%d", &j);
            adj_matrix[i][j] = 1;
        }
    fclose(edgelist_file);
    }
    return makeHashGraph(adj_matrix, imax + 1, jmax + 1);
}


std::vector<int> HashGraph::edgelist(){
    std::vector<int> edge_list = std::vector<int>();
    std::unordered_set<int>::iterator set_iter;
    for (int i = 0; i < m_nRows; i++) {
        if (transposed){
            for (set_iter = m_adjArr[i].begin(); set_iter != m_adjArr[i].end(); ++set_iter) {
                edge_list.push_back(*set_iter);
                edge_list.push_back(i);
            }
        } else {
            for (set_iter = m_adjArr[i].begin(); set_iter != m_adjArr[i].end(); ++set_iter) {
                edge_list.push_back(i);
                edge_list.push_back(*set_iter);
            }
        }
    }
    return edge_list;
}


void edgelist_to_file(std::vector<int> edgelist, char * filename) {
    std::ofstream out(filename);
    int i = 0;
    while (i < edgelist.size()) {
        out << edgelist[i] << '\t';
        i++;
        out << edgelist[i] << '\n';
        i ++;
    }
    out.close();
}


/* ### Testing ### */
// Function to randomize an individual graph.
void set_operation_tests() {
    int aArr[6]{1, 2, 3, 4, 5, 6};
    int bArr[6]{4, 5, 6, 7, 8, 9};
    std::unordered_set<int> a = std::unordered_set<int>();
    std::unordered_set<int> b = std::unordered_set<int>();
    for (int i = 0; i < 6; i++) {
        a.insert(aArr[i]);
        b.insert(bArr[i]);
    }
    std::unordered_set<int> aUb = setUnion(a, b);
    std::unordered_set<int> aIb = setIntersect(a, b);
    fprintf(stdout, "Union of a and b: ");
    std::unordered_set<int>::iterator aUb_iter;
    for (aUb_iter = aUb.begin(); aUb_iter != aUb.end(); ++aUb_iter) {
        fprintf(stdout, "%d ", *aUb_iter);
    }
    fprintf(stdout, "\n");

    fprintf(stdout, "Intersection of a and b: ");
    std::unordered_set<int>::iterator aIb_iter;
    for (aIb_iter = aIb.begin(); aIb_iter != aIb.end(); ++aIb_iter) {
        fprintf(stdout, "%d ", *aIb_iter);
    }
    fprintf(stdout, "\n");
}

void random_test() {
    fprintf(stdout, "Random 64-bit seed: %lu\n", getSeed());
    fprintf(stdout, "Random 64-bit number using 256-bit seed: %lu\n", next());
}

void HashGraph::swap_test() {
    // This is where the edges will be traded between two vertices in a random fashion.

    // Select two rows at random
    fprintf(stdout, "nRows: %d\n", nRows());
    fprintf(stdout, "nCols: %d\n", nCols());

    int row1 = next() % nRows();
    fprintf(stdout, "row1 number: %d\n", row1);
    int row2 = next() % (nRows() - 1);
    if (row2 == row1) row2 ++;
    fprintf(stdout, "row2 number: %d\n", row2);
    fprintf(stdout, "Row 1:");
    std::unordered_set<int>::iterator row1_iter;
    for (row1_iter = m_adjArr[row1].begin(); row1_iter != m_adjArr[row1].end(); ++row1_iter) {
        fprintf(stdout, " %d", *row1_iter);
    }
    fprintf(stdout, "\nRow 2:");
    std::unordered_set<int>::iterator row2_iter;
    for (row2_iter = m_adjArr[row2].begin(); row2_iter != m_adjArr[row2].end(); ++row2_iter) {
        fprintf(stdout, " %d", *row2_iter);
    }
    puts("");
    shuffle(row1, row2);
    fprintf(stdout, "Row 1:");
    for (row1_iter = m_adjArr[row1].begin(); row1_iter != m_adjArr[row1].end(); ++row1_iter) {
        fprintf(stdout, " %d", *row1_iter);
    }
    fprintf(stdout, "\nRow 2:");
    for (row2_iter = m_adjArr[row2].begin(); row2_iter != m_adjArr[row2].end(); ++row2_iter) {
        fprintf(stdout, " %d", *row2_iter);
    }
    puts("");
}

void HashGraph_test() {
    bool adjArr[6][5] {
        {0, 0, 0, 1, 0},
        {0, 1, 1, 1, 0},
        {1, 0, 0, 0, 1},
        {1, 1, 1, 1, 0},
        {1, 1, 0, 0, 1},
        {0, 0, 1, 1, 1},
    };
    puts("Assigned adjArr");
    HashGraph test = makeHashGraph<6, 5> (adjArr);
    puts("Created test HashGraph");
    test.swap_test();
}


int main(int argc, char *argv[]) {
    // 0. Parse command-line arguments.
    // 1. Read the graph file to form an adjacency matrix.
    // 2. Copy the adjacency matrix.
    // 3. Perform swaps on the copied adjacancy matrix.
    // 4. Construct an edge-list from the adjacency matrix.
    // 5. Write the edge list to an output file.

    // Notes: 2-5 will be done many times. Can also try to divide the copies into different threads.
    // Might want to experiment with using more space-efficient data structures.
    // Before making this multi-threaded, should try a single-threaded implementation and consider the amount of space required.
    //uint64_t s[4];
    set_seed();
    // TODO: Make this program parse input arguments better.
    if (argc == 1) {
        random_test();
        set_operation_tests();
        HashGraph_test();
        return 0;
    }
    char filepath[9] = "test.tsv";
    HashGraph target = read_edgelist(argv[1]);
    int swaps = std::stoi(argv[2]);
    char* outdir = argv[3];
    int ngraphs = std::stoi(argv[4]);
    fprintf(stdout, "%d swaps\n", swaps);
    //target.swap_n_times(swaps);
    std::vector<int> edge_list = target.edgelist();
    //edgelist_to_file(edge_list, outfile);
    for (int i = 0; i < ngraphs; i++) {
        //HashGraph* copy = target.copy();  // This seems to work...
        // Do something with copy...
        //copy->swap_n_times(swaps * copy->nRows());
        target.swap_n_times(swaps * target.nRows());
        std::string outfile = std::string(outdir) + "/random_graph." + std::to_string(i);
        //edgelist_to_file(copy->edgelist(), &outfile[0]);
        edgelist_to_file(target.edgelist(), &outfile[0]);
        //delete copy;
    }

    return 0;
}
