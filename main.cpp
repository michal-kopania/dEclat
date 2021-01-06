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

std::string out_filename = "", stat_filename = "", data_filename, taxonomy_filename = "";
unsigned int min_sup;
std::unordered_map<unsigned int, unsigned int> taxonomy; // Read from taxonomy file
taxonomy_tree hierarchy_tree;
/*For each item I look for its parent in taxonomy. I have noticed, that hierarchy at the botton level has many leafs, but at the level up number of leaves is very small. That’s why I have second set - parent_taxonomy. When I look for parent of item in dataset I search taxonomy, but to search for its parent I search in parent_taxonomy structure. It is faster that way.*/
std::unordered_map<unsigned int, std::set<unsigned int>> *vertical_representation;
tree tree;
unsigned int number_of_transactions = 0;
vector<string> stat_data; //Info to be saved in stat file.
unsigned int number_of_created_candidates = 0; //total # of created candidates,
unsigned int number_of_frequent_itemsets = 0;// total # of discovered frequent itemsets
std::map<unsigned int, pair<unsigned int, unsigned int>> number_of_created_candidates_and_frequent_itemsets_of_length; //For stats
//- # of created candidates of length 1, total # of discovered frequent itemsets of length 1

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

        hierarchy_tree.add(element, parent);

#if DEBUG_LEVEL > 1
        cout<<element<<","<<parent<<endl;
#endif
    }
#if DEBUG_LEVEL > 0
//    auto print = [](std::pair<const unsigned int, unsigned int> &n) {
//        std::cout << " " << n.first << ' (' << n.second << ')';
//    };
//    std::for_each(taxonomy.begin(), taxonomy.end(), print);
    hierarchy_tree.print();

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
            auto inserted = (*vertical_representation)[element].insert(t_id);
            if(use_taxonomy) {
                auto search = taxonomy.find(element);
                if(search != taxonomy.end()) {
#if DEBUG_LEVEL > 1
                    std::cout << endl<<"Found " << search->first << ", parent: " << search->second << '\n';
#endif
                    //Comment this if you want append all parents to transaction
                    (*vertical_representation)[search->second].insert(t_id);
#if DEBUG_LEVEL > 0
                    parent_elements.emplace(search->second);
#endif
                    /* I do not insert parents from hierarchy. I will later on display them
                     * Id abcdE is frequent than abcdEParent_E is also frequent
                     * Parent_E is also frequent Need to think it over*/
                    //Uncomment if you want put parents to transaction
                    /*
                    while(true) {
                        search = taxonomy.find(search->second);
                        if(search != taxonomy.end()) {
#if DEBUG_LEVEL > 1
                            std::cout << " -> " << search->first ;// << " " << search->second << '\n';
#endif
                            parent_elements.emplace(search->first);
                            (*vertical_representation)[search->first].insert(t_id);
                        } else {
                            break;
                        }
                    }
                    */
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
    auto print = [](const std::pair<const unsigned int, std::set<unsigned int>> &s) {
        std::cout << "item " << s.first << " in transactions (";
        std::for_each(s.second.begin(), s.second.end(), [](unsigned int x) {
            std::cout << x << ' ';
        });
        cout << ") size=" << s.second.size() << endl;
    };

    std::for_each(vertical_representation->cbegin(), vertical_representation->cend(), print);
#endif
    //Remove infrequent items from vertical representation
    // erase all with support <= min_sup
    number_of_created_candidates = vertical_representation->size();
    for(auto it = vertical_representation->begin(); it != vertical_representation->end();) {
        if(it->second.size() <= min_sup)
            it = vertical_representation->erase(it);
        else
            ++it;
    }
    number_of_frequent_itemsets = vertical_representation->size();
    number_of_created_candidates_and_frequent_itemsets_of_length[1] = pair(number_of_created_candidates,
                                                                           number_of_frequent_itemsets);
#if DEBUG_LEVEL > 0
    cout << endl << "After removing <= min_sup" << endl;
    std::for_each(vertical_representation->cbegin(), vertical_representation->cend(), print);
#endif
    return 1;
}

/**
 * First level has singletons itemsets
 */
void create_first_level_diff_sets()
{
    node *root_node;
    root_node = new node;
    tree.add(root_node, nullptr);
    tree.root = root_node;

    for(auto it = vertical_representation->begin(); it != vertical_representation->end(); ++it) {
        //it->second.size() it is support
        node *singleton;
        singleton = new node;
        singleton->element = it->first;
        singleton->support = it->second.size();
        //Create singleton->diff_set
        unsigned int first = *it->second.begin(); //First transaction id for element
        unsigned int last = *--it->second.end(); //Last transaction id
        auto diff_set_it = singleton->diff_set.begin();
        //Let's say that I have for item 100 transaction ids 5,8,12
        //Number of transactions is 20
        //So diff list {1,2,...,20} - {5,8,12} is ids before 5, ids after 12 and ids: {6,7,9,10,11}
        //Add transactions ids which are before first transaction in vertical_representation element
        for(unsigned int i = 1; i < first; ++i) {
            //This is faster than emplace() when you know where to add
            singleton->diff_set.emplace_hint(diff_set_it, i);
            diff_set_it = singleton->diff_set.end();
        }

        //Search and add. Start from second element. First transaction is not in diff list
        auto s_it = ++it->second.begin();
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

        tree.add(singleton, root_node);

        //Debug print
#if DEBUG_LEVEL > 0
        cout << endl << "Transactions for: " << it->first << endl;
        for(auto s_it = it->second.begin(); s_it != it->second.end(); ++s_it) {
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

//Nie potrzebne
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

//Można poprawić uwzględniając min_sup. Mniej porównań będzie wtedy
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

void traverse(node *pNode)
{
    for(auto it = pNode->children.begin(); it != pNode->children.end(); ++it) {
#if DEBUG_LEVEL > 1
        (*it)->print();
#endif
        auto brother = it;
        brother++; //For all right hand brothers
        for(auto b_it = brother; b_it != pNode->children.end(); ++b_it) {
            //Go deeper and create and calculate next level
            //For statistics
            ++number_of_created_candidates;
            auto child_level = (*it)->level + 1;
            std::cout << number_of_created_candidates << "," << number_of_frequent_itemsets << "\r" << std::flush;
#if DEBUG_LEVEL > 1
            cout<<" L: "<<(*it)->level<<" item: "<<(*it)->element<<" brother:"<<(*b_it)->element<<endl;
#endif
            auto search = number_of_created_candidates_and_frequent_itemsets_of_length.find(child_level);
            std::pair<std::map<unsigned int, pair<unsigned int, unsigned int>>::iterator, bool> inserted;
            if(search == number_of_created_candidates_and_frequent_itemsets_of_length.end()) {
                inserted = number_of_created_candidates_and_frequent_itemsets_of_length.emplace(child_level,
                                                                                                pair(1, 0));
                search = inserted.first;
            } else {
                (*search).second.first++; //Increase number of candidates
            }

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
                ++number_of_frequent_itemsets;
                (*search).second.second++; //Increase number of number_of_created_candidates_and_frequent_itemsets_of_length[level].second (which is frequent), first is candidate
                new_node->element = (*b_it)->element;
                new_node->support = sup;
                tree.add(new_node, *it);
#if DEBUG_LEVEL > 0
                cout << "Added: " << new_node->element << " at level: " << new_node->level << " to "
                     << new_node->parent->element << " from level: " << new_node->parent->level << endl;
#endif
            } else {
                delete new_node;
            }
        }
        traverse((*it));
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
            ("stat,s", po::value<std::string>(&stat_filename)->default_value(""), "stat filename");

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

    bool use_taxonomy;
    if(vm.count("taxonomy")) {
        taxonomy_filename = vm["taxonomy"].as<string>();
        cout << "taxonomy was set to: "
             << taxonomy_filename << "\n";
        stat_data.push_back("name of the hierarchy dataset: " + taxonomy_filename);
        auto s = get_wall_time();
        //cout<<"reading the hierarchy datasets start: "<<s<<endl;
        if(read_taxonomy(taxonomy_filename) < 0) {
            return 1;
        }
        auto e = get_wall_time();
        cout << "reading the hierarchy datasets: " << e - s << " sec." << endl;
        use_taxonomy = true;
        stat_data.push_back("reading the hierarchy datasets: " + to_string(e - s) + " sec.");
    } else {
        use_taxonomy = false;
        cout
                << "Argument taxonomy was not provided. Calculation will be performed without hierarchy. To calculate dEclat with hierarchy, provide taxonomy filename --taxonomy=taxonomy_data.txt\n";
    }

    if(vm.count("out")) {
        out_filename = vm["out"].as<string>();
        if(out_filename == "") {
            if(use_taxonomy) {
                out_filename = string("out_Hierarchy-dEclat_") + fs::path(data_filename.c_str()).stem().native() +
                               string("_m") + to_string(min_sup) + "_h" +
                               fs::path(taxonomy_filename.c_str()).stem().native() + ".txt";
            } else {
                //out_Hierarchy-dEclat_fname_m400_hName.txt
                out_filename = string("out_Hierarchy-dEclat_") + fs::path(data_filename.c_str()).stem().native() +
                               string("_m") + to_string(min_sup) + ".txt";
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
                                string("_m") + to_string(min_sup) + "_h" +
                                fs::path(taxonomy_filename.c_str()).stem().native() + ".txt";
            } else {
                //out_Hierarchy-dEclat_fname_m400_hName.txt
                stat_filename = string("stat_Hierarchy-dEclat_") + fs::path(data_filename.c_str()).stem().native() +
                                string("_m") + to_string(min_sup) + ".txt";
            }
        }
        cout << "stat was set to: "
             << stat_filename << "\n";
    } else {
        cout
                << "Argument stat was not provided. If you want stats to be saved to file, provide filename as argument --stat=stat.txt\n";
    }


    vertical_representation = new std::unordered_map<unsigned int, std::set<unsigned int>>;
    auto s = get_wall_time();
    read_dataset(data_filename, use_taxonomy);
    auto e = get_wall_time();
    cout << "reading and extending the dataset with transactions with hierarchical items: " << e - s << " sec." << endl;
    stat_data.push_back(
            "reading and extending the dataset with transactions with hierarchical items: " + to_string(e - s) +
            " sec.");

    //First level
    s = get_wall_time();
    create_first_level_diff_sets();
    hierarchy_tree.calculate_support();

    //To save some memory
    delete vertical_representation;
    e = get_wall_time();
    cout << "creation of diffLists for singleton itemsets: " << e - s << " sec." << endl;
    stat_data.push_back("creation of diffLists for singleton itemsets: " + to_string(e - s) + " sec.");

    s = get_wall_time();
    //Traverse taxonomy
    hierarchy_tree.print_frequent_itemset(out_filename);
    //I can delete hierarchy_tree here.

    //dEclat algo
    traverse(tree.root);

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
    tree.print_frequent_itemset(out_filename);
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
        myfile << "saving results to OUT and STAT files: " << e - s << " sec." << endl;
        myfile << "total runtime: " << e - start_time << " sec." << endl;
        myfile << "the number of items in a discovered frequent itemset of the greatest cardinality: "
               << tree.number_of_items_of_greatest_cardinality << endl;
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