#include <iostream>
#include <boost/program_options.hpp>
#include <unordered_map>
#include <fstream>
#include <sys/time.h>
#include <filesystem>
#include "tree.hpp"
#include "taxonomy_tree.hpp"

#define DEBUG_LEVEL 0

namespace fs = std::filesystem;
using namespace std;
namespace po = boost::program_options;

std::string out_filename = "", stat_filename = "", data_filename, taxonomy_filename = "", taxonomy_handling;
unsigned int min_sup;
std::unordered_map<unsigned int, unsigned int> taxonomy; // Read from taxonomy file
taxonomy_tree *hierarchy_tree;
/*For each item I look for its parent in taxonomy. I have noticed, that hierarchy at the botton level has many leafs, but at the level up number of leaves is very small. That’s why I have second set - parent_taxonomy. When I look for parent of item in dataset I search taxonomy, but to search for its parent I search in parent_taxonomy structure. It is faster that way.*/
std::unordered_map<unsigned int, std::set<unsigned int> *> *vertical_representation;
struct tree tree;
unsigned int number_of_transactions = 0;
vector<string> stat_data; //Info to be saved in stat file.
unsigned int number_of_created_candidates = 0; //total # of created candidates,
unsigned int number_of_frequent_itemsets = 0;// total # of discovered frequent itemsets
std::map<unsigned int, pair<unsigned int, unsigned int>> number_of_created_candidates_and_frequent_itemsets_of_length; //For stats
//- # of created candidates of length 1, total # of discovered frequent itemsets of length 1
bool are_items_mapped_to_string = false;
std::map<unsigned int, string> names_for_items;
double save_time = 0;

std::vector<std::string> split(std::string str, std::string sep, int limit)
{
    char *cstr = const_cast<char *>(str.c_str());
    char *current;
    std::vector<std::string> arr;
    current = strtok(cstr, sep.c_str());
    int count = 0;
    while(current != NULL) {
        ++count;
        if(count > limit && limit != -1) {
            break;
        }
        arr.push_back(current);
        current = strtok(NULL, sep.c_str());
    }
    return arr;
}

std::vector<std::string> split(const std::string &s, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while(std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

double get_wall_time()
{
    struct timeval time;
    if(gettimeofday(&time, NULL)) {
        //  Handle error
        return 0;
    }
    return (double) time.tv_sec + (double) time.tv_usec * .000001;
    /*
    auto start = std::chrono::system_clock::now();
    auto stop = std::chrono::system_clock::now();
    std::chrono::duration<double, std::milli> time = stop - start;
    */
}

double get_cpu_time()
{
    return (double) clock() / (double) CLOCKS_PER_SEC;
}


int read_taxonomy(const string &taxonomy_filename)
{
    hierarchy_tree = new taxonomy_tree;
    fstream file(taxonomy_filename, ios::in);
    if(!file.is_open()) {
        cout << "File " << taxonomy_filename << " not found!" << endl;
        return -1;
    }
    string line;
    while(getline(file, line)) {
        std::stringstream linestream(line);
        std::string value;
        unsigned int element, parent;
        bool first = true;
        while(getline(linestream, value, ',')) {
            if(first) {
                element = std::stoul(value, nullptr, 0);
                first = false;
            } else {
                parent = std::stoul(value, nullptr, 0);
            }
        }
        if(first) {
            continue; //Line is empty
        }
        taxonomy.emplace(element, parent);
        if(taxonomy_handling == "separate") {
            hierarchy_tree->add(element, parent);
        }

#if DEBUG_LEVEL > 1
        cout<<element<<","<<parent<<endl;
#endif
    }
#if DEBUG_LEVEL > 0
//    auto print = [](std::pair<const unsigned int, unsigned int> &n) {
//        std::cout << " " << n.first << ' (' << n.second << ')';
//    };
//    std::for_each(taxonomy.begin(), taxonomy.end(), print);
    hierarchy_tree->print();

#endif

    return 1;
}

int read_dataset(const string &filename, bool use_taxonomy)
{
    fstream file(filename, ios::in);
    if(!file.is_open()) {
        cout << "File " << filename << " not found!" << endl;
        return -1;
    }
    string line;
    unsigned int t_id = 0; //Transaction id
    while(getline(file, line)) {
        std::stringstream linestream(line);
        std::string value;
        std::set<unsigned int> parent_elements; //For debug
        unsigned int element;
        if(line[0] == '@') {
            //map ids to string
            line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
            line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
            vector<string> tokens;
            tokens = split(line, "=", 3);
            if(tokens.size() == 3) {
                are_items_mapped_to_string = true;
                unsigned int id;
                id = std::stoul(tokens[1]);
                names_for_items[id] = tokens[2];
            } else {
                cerr << "format error. Should be @ITEM=N=S where N - unsigned int, S - string. Parsed line is: " << line
                     << endl;
                return -1;
            }
            continue;
        }
        ++t_id;
#if DEBUG_LEVEL > 0
        cout << "[" << t_id << "] ";
#endif
        while(getline(linestream, value, ' ')) {
            element = std::stoul(value, nullptr, 0);
#if DEBUG_LEVEL > 0
            cout << element;
#endif
            //std::pair<std::map<unsigned int, pair<unsigned int, unsigned int>>::iterator, bool> inserted;
            auto search = vertical_representation->find(element);
            if(search != vertical_representation->end()) {
                //std::cout << "Found " << search->first << " " << search->second << '\n';
                search->second->insert(t_id);
            } else {
                //std::cout << "Not found\n";
                std::set<unsigned int> *s;
                s = new std::set<unsigned int>;
                s->insert(t_id);
                vertical_representation->insert_or_assign(element, s);//
            }

            if(use_taxonomy && taxonomy_handling == "in_tids") {
                auto search = taxonomy.find(element);
                if(search != taxonomy.end()) {
#if DEBUG_LEVEL > 1
                    std::cout << endl<<"Found " << search->first << ", parent: " << search->second << '\n';
#endif
                    //Comment this if you want append all parents to transaction
                    //(*vertical_representation)[search->second]->insert(t_id);
                    /* I do not insert parents from hierarchy. I will later on display them
                     * Id abcdE is frequent than abcdEParent_E is also frequent
                     * Parent_E is also frequent Need to think it over*/
                    //Uncomment if you want put parents to transaction

                    while(search != taxonomy.end()) {
                        auto parent = search->second;
                        search = taxonomy.find(parent); //Search for parent
#if DEBUG_LEVEL > 1
                        std::cout << " -> " << search->first ;// << " " << search->second << '\n';
#endif
#if DEBUG_LEVEL > 0
                        parent_elements.emplace(parent);
#endif
                        //(*vertical_representation)[search->first].insert(t_id);
                        auto search2 = vertical_representation->find(parent);
                        if(search2 != vertical_representation->end()) {
                            search2->second->insert(t_id);
                        } else {
                            std::set<unsigned int> *s;
                            s = new std::set<unsigned int>;
                            s->insert(t_id);
                            vertical_representation->insert_or_assign(parent, s);//Add parent to vertical_representation
                        }
                    }

                } else {
#if DEBUG_LEVEL > 1
                    std::cout << "Not found\n";
#endif
                }
            }
#if DEBUG_LEVEL > 0
            cout << ", ";
#endif
        }
        //elements of transaction printed out
#if DEBUG_LEVEL > 0
        if(use_taxonomy) {
            std::for_each(parent_elements.cbegin(), parent_elements.cend(), [](unsigned int x) {
                std::cout << x << "; "; //printing parents from hierarchy
            });
        }
        cout << endl; //elements and taxonomy printed. Get next transaction
#endif
    }
    number_of_transactions = t_id;
#if DEBUG_LEVEL > 0
    //Lets print it out
    auto print = [](const std::pair<const unsigned int, std::set<unsigned int> *> &s) {
        std::cout << "item " << s.first << " in transactions (";
        std::for_each(s.second->begin(), s.second->end(), [](unsigned int x) {
            std::cout << x << ' ';
        });
        cout << ") size=" << s.second->size() << endl;
    };

    std::for_each(vertical_representation->cbegin(), vertical_representation->cend(), print);
#endif
    //I can remove infrequent items from vertical representation
    // erase all with support <= min_sup
//    number_of_created_candidates = vertical_representation->size();
//    for(auto it = vertical_representation->begin(); it != vertical_representation->end(); ++it) {
//        if(it->second->size() > min_sup) {
//            ++number_of_frequent_itemsets;
//        }
//    }
//    number_of_created_candidates_and_frequent_itemsets_of_length[1] = pair(number_of_created_candidates,
//                                                                           number_of_frequent_itemsets);

    return 1;
}

/**
 * First level has singletons itemsets
 */
void create_first_level_diff_sets(struct tree &t,
                                  std::unordered_map<unsigned int, std::set<unsigned int> *> *vertical_representation)
{
    node *root_node;
    root_node = new node;
    t.add(root_node, nullptr);
    t.root = root_node;
    cout << "create_first_level_diff_sets()" << endl;
    for(auto it = vertical_representation->begin(); it != vertical_representation->end(); ++it) {
        //For stats
        ++number_of_created_candidates;
        //std::cout << number_of_created_candidates << "," << number_of_frequent_itemsets << "\r" << std::flush;
        auto search = number_of_created_candidates_and_frequent_itemsets_of_length.find(1);
        std::pair<std::map<unsigned int, pair<unsigned int, unsigned int>>::iterator, bool> inserted;
        if(search == number_of_created_candidates_and_frequent_itemsets_of_length.end()) {
            inserted = number_of_created_candidates_and_frequent_itemsets_of_length.emplace(1,
                                                                                            pair(1, 0));
            search = inserted.first;
        } else {
            (*search).second.first++; //Increase number of candidates
        }
        //stats end

        //Is frequent?
        if(it->second->size() <= min_sup) {
            continue; //No. Go to next
        }
        //for stats
        ++number_of_frequent_itemsets;
        (*search).second.second++; //Increase number of number_of_created_candidates_and_frequent_itemsets_of_length[level].second (which is frequent), first is candidate
        //stats end

        cout << it->first << "    \r";
        //it->second.size() it is support
        node *singleton;
        singleton = new node;
        singleton->element = it->first;
        singleton->support = it->second->size();
        //Create singleton->diff_set
        unsigned int first = *it->second->begin(); //First transaction id for element
        unsigned int last = *--it->second->end(); //Last transaction id
        auto diff_set_it = singleton->diff_set.begin();
        //Let's say that I have for item 100 transaction ids 5,8,12
        //Number of transactions is 20
        //So diff list {1,2,...,20} - {5,8,12} is ids before 5, ids after 12 and ids: {6,7,9,10,11}
        //Add transactions ids which are before first transaction in vertical_representation element
        //TODO: Instead off adding elements to diff_set add first and last. Later on generate diff_set
        for(unsigned int i = 1; i < first; ++i) {
            //This is faster than emplace() when you know where to add
            singleton->diff_set.emplace_hint(diff_set_it, i);
            diff_set_it = singleton->diff_set.end();
        }

        //Search and add. Start from second element. First transaction is not in diff list
        auto s_it = ++it->second->begin();
        for(unsigned int i = first + 1; i <= last; ++i) {
            if(i == *s_it) {
                //Do NOT add to diff_set
                ++s_it; //Go to next element
            } else if(i < *s_it) {
                //Add i to diff_set
                singleton->diff_set.emplace_hint(diff_set_it, i);
                diff_set_it = singleton->diff_set.end();
            }
        }

        //Add ids which are after last element
        for(unsigned int i = last + 1; i <= number_of_transactions; ++i) {
            //This is faster than emplace()
            singleton->diff_set.emplace_hint(diff_set_it, i);
            diff_set_it = singleton->diff_set.end();
        }

        t.add(singleton, root_node);

        //Debug print
#if DEBUG_LEVEL > 0
        cout << endl << "Transactions for: " << it->first << endl;
        for(auto s_it = it->second->begin(); s_it != it->second->end(); ++s_it) {
            cout << *s_it << ", ";
        }
        cout << endl << "diff_set: " << endl;
        for(auto d_it = singleton->diff_set.begin(); d_it != singleton->diff_set.end(); ++d_it) {
            cout << *d_it << ", ";
        }
        cout << endl;
#endif
    }

}


void
common(const std::set<unsigned int> &diff_set1, const std::set<unsigned int> &diff_set2, std::set<unsigned int> &result)
{
    auto it1 = diff_set1.begin(), it2 = diff_set2.begin();
    auto hint_it = result.begin();
    while(it1 != diff_set1.end() && it2 != diff_set2.end()) {
        if(*it1 == *it2) {
            //Add to result
            result.emplace_hint(hint_it, *it1);
            hint_it = result.end();
            ++it1;
            ++it2;
        } else if(*it1 < *it2) {
            ++it1; //Go to next element
        } else {
            ++it2;
        }
    }
}

//TODO: Można poprawić uwzględniając min_sup. Mniej porównań będzie wtedy
void difference(const std::set<unsigned int> &diff_set1, const std::set<unsigned int> &diff_set2,
                std::set<unsigned int> &result)
{
    auto it1 = diff_set1.begin(), it2 = diff_set2.begin();
    auto hint_it = result.begin();
    while(it1 != diff_set1.end() && it2 != diff_set2.end()) {
        if(*it1 == *it2) {
            ++it1;
            ++it2;
        } else if(*it1 < *it2) {
            //Add to result
            result.emplace_hint(hint_it, *it1);
            hint_it = result.end();
            ++it1; //Go to next element
        } else {
            ++it2;
        }
    }
    for(; it1 != diff_set1.end(); ++it1) {
        result.emplace_hint(hint_it, *it1);
        hint_it = result.end();
    }
}

void print_frequent_itemset_and_delete_node(node *pNode, const string &file, bool sorted)
{
    string ascendant = "";
    ofstream myfile;
    if(file != "") {
        myfile.open(file, fstream::out | fstream::app);
        if(!myfile) {
            cout << "Cannot open file: " << file << endl << "Results will not be saved and be printed at stdio instead"
                 << endl;
            myfile.close();
        } else {
            //myfile << "length\tsup\tdiscovered_frequent_itemset" << endl;
        }
    }
    auto s = get_wall_time();
    if(myfile.is_open()) {
        myfile << pNode->level << "\t" << pNode->support << "\t" << pNode->element << "" << endl;
    } else {
        cout << pNode->level << "\t" << pNode->support << "\t" << pNode->element << "" << endl;
    }

    if(sorted) {
        std::set<unsigned int> set_ascendant;
        set_ascendant.insert(pNode->element);
        tree.print_frequent_itemset_sorted(pNode, set_ascendant, myfile);
    } else {
        ascendant = to_string(pNode->element);
        tree.print_frequent_itemset(pNode, ascendant, myfile);
    }
    if(myfile.is_open()) {
        myfile.close();
    }
    auto e = get_wall_time();
    save_time += e - s;

    delete pNode;
}

void traverse(node *pNode, struct tree &t, taxonomy_tree *t_taxonomy, const string &file, bool sorted)
{
    //TODO: Wykorzystać t_taxonomy?
    //Jeśli t_taxonomy nie jest null to można sprawdzać czy right hand brother to nie jest czasem parent
    //Jeśli jest to parent to nie ma sensu liczyć ponownie
    //Biorę b_it i sprawdzam czy it to nie jest parent b_it lub na odwrót (?)
    //Jeśli b_it to parent it, to we b_it jest we tych samych lub w więcej transakcjac jak it
    //Gdy it jest frequent to it,b_it też jest frequent
    //diff list: D(it;b_it) = D(b_it) - D(it)
    //D(it;b_it) = T(it) - T(b_it) = empty, bo it zawiera się w b_it
    //support = (*it)->support - diff_set.size() = (*it)->support - 0 = (*it)->support
    //Jeśli it to parent b_it to
    //diff list:
    //D(it;b_it) = T(it) - T(b_it) = trzeba policzyć
    //Niech to będzie N elementów pochodzących z innych dzieci.
    //support = (*b_it)->support - N
    //(*b_it)->support jest o N większe od (*it)->support
    //Czyli: support = (*b_it)->support - N = (*it)->support + N - N = (*it)->support
    //Jeśli sobie ustawię tak elementy że najpierw będę miał parent to będę mógł znacznie zmniejszyć obliczenia.
    //Tylko, że difference() zakłada posortowane elementy. Zatem root powinien w hierarchi mieć najmniejszą wartość
    auto prev_it = pNode->children.begin();
    for(auto it = pNode->children.begin(); it != pNode->children.end(); ++it) {
        if(it != pNode->children.begin()) {
            prev_it = it;
        }
#if DEBUG_LEVEL > 1
        (*it)->print();
#endif
        if(*it == nullptr) {
            continue;
        }
        if((*it)->support > min_sup) {
            auto brother = it;
            brother++; //For all right hand brothers
            for(auto b_it = brother; b_it != pNode->children.end(); ++b_it) {
                //For statistics
                ++number_of_created_candidates;
                auto child_level = (*it)->level + 1;
                std::cout << number_of_created_candidates << "," << number_of_frequent_itemsets << "\r" << std::flush;
#if DEBUG_LEVEL > 1
                cout<<" L: "<<(*it)->level<<" item: "<<(*it)->element<<" brother:"<<(*b_it)->element<<endl;
#endif
                //For stats
                auto search = number_of_created_candidates_and_frequent_itemsets_of_length.find(child_level);
                std::pair<std::map<unsigned int, pair<unsigned int, unsigned int>>::iterator, bool> inserted;
                if(search == number_of_created_candidates_and_frequent_itemsets_of_length.end()) {
                    inserted = number_of_created_candidates_and_frequent_itemsets_of_length.emplace(child_level,
                                                                                                    pair(1, 0));
                    search = inserted.first;
                } else {
                    (*search).second.first++; //Increase number of candidates
                }
                //stats end

                //Is it frequent?
                if((*b_it)->support <= min_sup) {
                    continue;
                }
                //Go deeper and create and calculate next level

                node *new_node;
                new_node = new node;
                //D(ab) = D(b) - D(a)
                //D(ab) = T(a) - T(b)
                //I have diff list, so
                difference((*b_it)->diff_set, (*it)->diff_set, new_node->diff_set);
                //if sup > minSup add to tree
                //Calculate sup sup(a) - len(D(ab))
                auto sup = (*it)->support - new_node->diff_set.size();
                if(sup > min_sup) {
                    //for stats
                    ++number_of_frequent_itemsets;
                    (*search).second.second++; //Increase number of number_of_created_candidates_and_frequent_itemsets_of_length[level].second (which is frequent), first is candidate
                    //for stats end
                    new_node->element = (*b_it)->element;
                    new_node->support = sup;
                    t.add(new_node, *it);
#if DEBUG_LEVEL > 0
                    cout << "Added: " << new_node->element << " at level: " << new_node->level << " to "
                         << new_node->parent->element << " from level: " << new_node->parent->level << endl;
#endif
                } else {
                    delete new_node;
                }
            }
            traverse((*it), t, t_taxonomy, file, sorted);
        }
#if DEBUG_LEVEL > 1
        cout << "---------- free memory --------" <<endl;
        (*it)->print();
        cout << "---------- free memory --------" <<endl;
#endif
        if(*it != nullptr) {
            (*it)->diff_set.clear();
        }
        if(it != pNode->children.begin()) {
            if((*prev_it)->level == 1) {
                print_frequent_itemset_and_delete_node((*prev_it), file, sorted);
                *prev_it = nullptr;
            }
        }
        /*
        (*it)->print();
         */
    }

}

void print_out_file_header(const std::string &file)
{
    ofstream myfile;
    if(file != "") {
        myfile.open(file, fstream::out | fstream::trunc);
        if(!myfile) {
            cerr << "Cannot open file: " << file << endl;
            myfile.close();
        } else {
            myfile << "length\tsup\tdiscovered_frequent_itemset" << endl;
        }
    }
    if(myfile.is_open()) {
        myfile.close();
    }
}


int main(int argc, const char **argv)
{

    auto start_time = get_wall_time();
    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
            ("help,h", "produce help message")
            ("min_sup,m", po::value<int>(), "minimum support")
            ("data_set,d", po::value<std::string>(), "data set file")
            ("taxonomy,t", po::value<std::string>(), "taxonomy filename")
            ("out,o", po::value<std::string>(&out_filename)->default_value(""), "out filename")
            ("stat,s", po::value<std::string>(&stat_filename)->default_value(""), "stat filename")
            ("sorted,r", po::value<bool>(), "if set to 1 then sort discovered frequent items in output")
            ("taxonomy_handling,th", po::value<std::string>(&taxonomy_handling)->default_value("in_tids"),
             "taxonomy handling. Values: in_tids - all ascendants are added to transactions\n separate - Taxonomy is treated separately. Frequent item sets are generated from transactions and taxonomy separately\nDefault value: in_tids");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if(vm.count("help")) {
        cout << desc << "\n";
        return 1;
    }

    if(vm.count("min_sup")) {
        min_sup = vm["min_sup"].as<int>();
        cout << "Minimum support level was set to: "
             << min_sup << "\n";
        stat_data.push_back("minSup: " + to_string(min_sup));
    } else {
        cout << "Minimum support was not set. Provide as an argument --min_sup=2\n";
        return 1;
    }

    if(vm.count("data_set")) {
        data_filename = vm["data_set"].as<string>();
        cout << "data_set was set to: "
             << data_filename << "\n";
        stat_data.push_back("name of the transaction dataset: " + data_filename);
    } else {
        cout << "Argument data_set was not provided. Provide as filename to data --data_set=data.txt\n";
        return 1;
    }

    bool items_sorted = false;
    std::string s_sorted = "";
    if(vm.count("sorted")) {
        items_sorted = vm["sorted"].as<bool>();
        cout << "sorted was set to: "
             << items_sorted << "\n";
        stat_data.push_back("sorted was set to: " + to_string(items_sorted));
        s_sorted = "_sorted_" + to_string(items_sorted);
    } else {
        s_sorted = "_sorted_0";
    }

    bool use_taxonomy;
    if(vm.count("taxonomy")) {
        taxonomy_filename = vm["taxonomy"].as<string>();
        cout << "taxonomy was set to: "
             << taxonomy_filename << "\n";
        stat_data.push_back("name of the hierarchy dataset: " + taxonomy_filename);
        use_taxonomy = true;
    } else {
        use_taxonomy = false;
        cout
                << "Argument taxonomy was not provided. Calculation will be performed without hierarchy. To calculate dEclat with hierarchy, provide taxonomy filename --taxonomy=taxonomy_data.txt\n";
    }

    if(use_taxonomy) {
        if(vm.count("taxonomy_handling")) {
            taxonomy_handling = vm["taxonomy_handling"].as<string>();
            cout << "taxonomy_handling was set to: "
                 << taxonomy_handling << "\n";
            stat_data.push_back("taxonomy_handling: " + taxonomy_handling);
        }
        auto s = get_wall_time();
        //cout<<"reading the hierarchy datasets start: "<<s<<endl;
        if(read_taxonomy(taxonomy_filename) < 0) {
            return 1;
        }
        auto e = get_wall_time();
        cout << "reading the hierarchy datasets: " << e - s << " sec." << endl;
        stat_data.push_back("reading the hierarchy datasets: " + to_string(e - s) + " sec.");
    }

    if(vm.count("out")) {
        out_filename = vm["out"].as<string>();
        if(out_filename == "") {
            if(use_taxonomy) {
                out_filename = string("out_Hierarchy-dEclat_") + fs::path(data_filename.c_str()).stem().native() +
                               string("_m") + to_string(min_sup) + "_h";
                out_filename += fs::path(taxonomy_filename.c_str()).stem().native();
                if(taxonomy_handling == "separate") {
                    out_filename += "_sep";
                }
                out_filename += s_sorted + ".txt";
            } else {
                //out_Hierarchy-dEclat_fname_m400_hName.txt
                out_filename = string("out_Hierarchy-dEclat_") + fs::path(data_filename.c_str()).stem().native() +
                               string("_m") + to_string(min_sup) + s_sorted + ".txt";
            }
        }
        cout << "out was set to: "
             << out_filename << "\n";
    } else {
        cout
                << "Argument out was not provided. If you want output to be saved to file, provide filename as argument --out=out.txt\n";
    }

    if(vm.count("stat")) {
        stat_filename = vm["stat"].as<string>();
        if(stat_filename == "") {
            if(use_taxonomy) {
                stat_filename = string("stat_Hierarchy-dEclat_") + fs::path(data_filename.c_str()).stem().native() +
                                string("_m") + to_string(min_sup) + "_h";
                stat_filename += fs::path(taxonomy_filename.c_str()).stem().native();
                if(taxonomy_handling == "separate") {
                    stat_filename += "_sep";
                }
                stat_filename += s_sorted + ".txt";
            } else {
                //out_Hierarchy-dEclat_fname_m400_hName.txt
                stat_filename = string("stat_Hierarchy-dEclat_") + fs::path(data_filename.c_str()).stem().native() +
                                string("_m") + to_string(min_sup) + s_sorted + ".txt";
            }
        }
        cout << "stat was set to: "
             << stat_filename << "\n";
    } else {
        cout
                << "Argument stat was not provided. If you want stats to be saved to file, provide filename as argument --stat=stat.txt\n";
    }

    vertical_representation = new std::unordered_map<unsigned int, std::set<unsigned int> *>;
    auto s = get_wall_time();
    int ret = read_dataset(data_filename, use_taxonomy);
    if(ret == -1) {
        return -1;
    }
    auto e = get_wall_time();
    cout << "reading and extending the dataset with transactions with hierarchical items: " << e - s << " sec." << endl;
    stat_data.push_back(
            "reading and extending the dataset with transactions with hierarchical items: " + to_string(e - s) +
            " sec.");

    //First level
    s = get_wall_time();
    create_first_level_diff_sets(tree, vertical_representation);
    e = get_wall_time();
    cout << "creation of diffLists for singleton itemsets: " << e - s << " sec." << endl;
    stat_data.push_back("creation of diffLists for singleton itemsets: " + to_string(e - s) + " sec.");

    print_out_file_header(out_filename);

    unsigned int taxonomy_greatest_cardinality = 0;
    unsigned int taxonomy_number_of_items_of_greatest_cardinality = 0;
    //Traverse taxonomy
/* I added parents to tid_ids if taxonomy_handling == "in_tids" */
    if(use_taxonomy && taxonomy_handling == "separate") {
        s = get_wall_time();
        cout << "Calculate support for taxonomy" << endl;
        hierarchy_tree->calculate_support(vertical_representation);
//    Do not need to print tree because it will be printed by tree_for_hierarchy.print_frequent_itemset()
//    hierarchy_tree.print_frequent_itemset(out_filename); //Prints singleton frequent_itemset from tree
        cout << "create_vertical_representation()" << endl;
        hierarchy_tree->create_vertical_representation();
        auto levels = (*(--hierarchy_tree->tree_vertical_representation.end())).first;
        for(auto it = hierarchy_tree->tree_vertical_representation.begin();
            it != hierarchy_tree->tree_vertical_representation.end(); ++it) {
            struct tree tree_for_hierarchy;
            create_first_level_diff_sets(tree_for_hierarchy, (*it).second);
            cout << "traverse for taxonomy for level: " << (*it).first << endl;
            //TODO: hierarchy_tree may be used to speed up the process.
            //I do not have to calculate list: A, B (parent of A), C (parent of B) if list: A was already calculated.
            traverse(tree_for_hierarchy.root, tree_for_hierarchy, hierarchy_tree, out_filename, items_sorted);
            tree_for_hierarchy.print_frequent_itemset(out_filename, items_sorted);
            auto l = (*it).first - 1;
        }

        hierarchy_tree->clear_sets_in_nodes();
        delete hierarchy_tree; //Free memory
        e = get_wall_time();
        cout
                << "Taxonomy creation of candidates for frequent itemsets as well as calculation of their diffLists and supports: "
                << e - s << " sec." << endl;
        stat_data.push_back(
                "Taxonomy creation of candidates for frequent itemsets as well as calculation of their diffLists and supports: " +
                to_string(e - s) + " sec.");
    }

    s = get_wall_time();
    //dEclat algo
    traverse(tree.root, tree, nullptr, out_filename, items_sorted);

    //I can delete hierarchy_tree here too
    for(auto it = vertical_representation->begin(); it != vertical_representation->end(); ++it) {
        delete it->second;
    }
    delete vertical_representation;

    e = get_wall_time();
    cout << "creation of candidates for frequent itemsets as well as calculation of their diffLists and supports: "
         << e - s << " sec." << endl;
    stat_data.push_back(
            "creation of candidates for frequent itemsets as well as calculation of their diffLists and supports: " +
            to_string(e - s) + " sec.");
#if DEBUG_LEVEL > 1
    cout << "------------ SEARCH TREE ------------" << endl;
    tree.print();
    cout << "-------------------------------------" << endl;
#endif

    s = get_wall_time();
    // If printed in traverse than cannot be printed here
    tree.print_frequent_itemset(out_filename, items_sorted);
    ofstream myfile;
    if(stat_filename != "") {
        myfile.open(stat_filename, fstream::out | fstream::trunc);
        if(!myfile) {
            cout << "Cannot open file: " << stat_filename << endl << "STATs will not be saved"
                 << endl;
            myfile.close();
        } else {
            for(auto it = stat_data.begin(); it != stat_data.end(); ++it) {
                myfile << (*it) << endl;
            }
        }
    }
    e = get_wall_time();
    if(myfile.is_open()) {
        myfile << "saving results to OUT and STAT files: " << save_time + e - s << " sec." << endl;
        myfile << "total runtime: " << e - start_time << " sec." << endl;

        unsigned int number_of_items_of_greatest_cardinality;
        if(!number_of_created_candidates_and_frequent_itemsets_of_length.empty()) {
            for(auto rit = number_of_created_candidates_and_frequent_itemsets_of_length.crbegin();
                rit != number_of_created_candidates_and_frequent_itemsets_of_length.crend(); ++rit) {
                if((*rit).second.second > 0) {
                    number_of_items_of_greatest_cardinality = (*rit).second.second;
                    break;
                }
            }
        }
        myfile << "the number of items in a discovered frequent itemset of the greatest cardinality: "
               << number_of_items_of_greatest_cardinality << endl;
        myfile << "total # of created candidates: " << number_of_created_candidates << endl;
        myfile << "total # of discovered frequent itemsets: " << number_of_frequent_itemsets << endl;
        for(auto it = number_of_created_candidates_and_frequent_itemsets_of_length.begin();
            it != number_of_created_candidates_and_frequent_itemsets_of_length.end(); ++it) {
            myfile << "# of created candidates of length " << (*it).first << ": " << (*it).second.first
                   << " total # of discovered frequent itemsets: " << (*it).second.second << endl;
        }
        myfile.close();
    }
    cout << "saving results to OUT and STAT files: " << e - s << " sec." << endl;
    return 0;
}


/*
    {
        vector<set<unsigned int>> vector1;
        set<unsigned int> intersect_set;
        set<unsigned int> set1 = {3, 6};
        vector1.push_back(set1);
        set1 = {1, 2, 3, 4, 5, 6};
        vector1.push_back(set1);
        set1 = {2, 6};
        vector1.push_back(set1);
        test_intersect(vector1, intersect_set);
        cout<<intersect_set.size()<<" {";
        for(auto it=intersect_set.begin();it!=intersect_set.end();++it){
            cout<<*it<<", ";
        }
        cout<<"}"<<endl;
    }
    {
        vector<set<unsigned int>> children;
        set<unsigned int> intersect_set;
        set<unsigned int> set1 = {3, 6};
        children.push_back(set1);
        set1 = {1, 2, 3, 4, 5};
        children.push_back(set1);
        set1 = {2, 7};
        children.push_back(set1);
        test_intersect(children, intersect_set);
        cout<<intersect_set.size()<<" {";
        for(auto it=intersect_set.begin();it!=intersect_set.end();++it){
            cout<<*it<<", ";
        }
        cout<<"}"<<endl;
    }
    {
        vector<set<unsigned int>> children;
        set<unsigned int> intersect_set;
        set<unsigned int> set1 = {3, 6};
        children.push_back(set1);
        set1 = {1, 2, 3, 4, 5, 6};
        children.push_back(set1);
        set1 = {2, 3, 6};
        children.push_back(set1);
        test_intersect(children, intersect_set);
        cout<<intersect_set.size()<<" {";
        for(auto it=intersect_set.begin();it!=intersect_set.end();++it){
            cout<<*it<<", ";
        }
        cout<<"}"<<endl;
    }
    {
        vector<set<unsigned int>> children;
        set<unsigned int> intersect_set;
        set<unsigned int> set1 ={1, 3, 4, 6};
        children.push_back(set1);
        set1 = {1, 2, 3, 4, 5, 6};
        children.push_back(set1);
        set1 = {1, 2, 3, 6};
        children.push_back(set1);
        test_intersect(children, intersect_set);
        cout<<intersect_set.size()<<" {";
        for(auto it=intersect_set.begin();it!=intersect_set.end();++it){
            cout<<*it<<", ";
        }
        cout<<"}"<<endl;
    }
*/