#include <iostream>
#include <boost/program_options.hpp>
#include <unordered_map>
#include "tree.hpp"
#include <fstream>
#include <sys/time.h>

using namespace std;
namespace po = boost::program_options;

std::string out_filename, stat_filename, data_filename, taxonomy_filename = "";
unsigned int min_sup;
std::unordered_map<unsigned int, unsigned int> taxonomy, parent_taxonomy;
std::unordered_map<unsigned int, std::set<unsigned int>> vertical_representation;
tree tree;
unsigned int number_of_transactions = 0;

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
        taxonomy.emplace(element, parent);
        auto search = parent_taxonomy.find(element);
        if(search != parent_taxonomy.end()) {
            //element is a parent of ot
            if(search->second == 0) {
                search->second = parent;
            }
            //std::cout << search->first << " -> " << parent_taxonomy.at(search->first) << endl;
        } else {
            search = parent_taxonomy.find(parent);
            if(search == parent_taxonomy.end()) {
                parent_taxonomy.emplace(parent, 0);
            }
        }
        //cout<<element<<","<<parent<<endl;
    }
    auto print = [](std::pair<const unsigned int, unsigned int> &n) {
        std::cout << " " << n.first << '(' << n.second << ')';
    };
    std::for_each(parent_taxonomy.begin(), parent_taxonomy.end(), print);
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
        std::set<unsigned int> parent_elements;
        unsigned int element;
        if(line[0] == '@') {
            continue;
        }
        ++t_id;
        cout << "[" << t_id << "] ";
        while(getline(linestream, value, ' ')) {
            element = std::stoul(value, nullptr, 0);
            cout << element;
            auto inserted = vertical_representation[element].insert(t_id);
            if(use_taxonomy) {
                auto search = taxonomy.find(element);
                if(search != taxonomy.end()) {
                    //      std::cout << "Found " << search->first << " " << search->second << '\n';
                    while(true) {
                        search = parent_taxonomy.find(search->second);
                        if(search != taxonomy.end()) {
                            //std::cout << " -> " << search->first ;// << " " << search->second << '\n';
                            parent_elements.emplace(search->first);
                            vertical_representation[search->first].insert(t_id);
                        } else {
                            break;
                        }
                    }
                } else {
                    //      std::cout << "Not found\n";
                }
            }
            cout << ", ";
        }
        //elements of transaction printed out
        //cout<<endl;
        if(use_taxonomy) {
            std::for_each(parent_elements.cbegin(), parent_elements.cend(), [](unsigned int x) {
                std::cout << x << "; "; //printing parents from hierarchy
            });
        }
        cout << endl; //elements and taxonomy printed. Get next transaction
    }
    number_of_transactions = t_id;
    //Lets print it out
    auto print = [](const std::pair<const unsigned int, std::set<unsigned int>> &s) {
        std::cout << "item " << s.first << " in transactions (";
        std::for_each(s.second.begin(), s.second.end(), [](unsigned int x) {
            std::cout << x << ' ';
        });
        cout << ") size=" << s.second.size() << endl;
    };

    std::for_each(vertical_representation.cbegin(), vertical_representation.cend(), print);

    //Remove infrequent items from vertical representation
    // erase all with support <= min_sup
    for(auto it = vertical_representation.begin(); it != vertical_representation.end();) {
        if(it->second.size() <= min_sup)
            it = vertical_representation.erase(it);
        else
            ++it;
    }
    cout << endl << "After removing <= min_sup" << endl;
    std::for_each(vertical_representation.cbegin(), vertical_representation.cend(), print);

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

    for(auto it = vertical_representation.begin(); it != vertical_representation.end(); ++it) {
        //it->second.size() it is support
        node *singleton;
        singleton = new node;
        singleton->element = it->first;
        singleton->support = it->second.size();
        //Create singleton->diff_set
        unsigned int first = *it->second.begin(); //First transaction id for element
        unsigned int last = *--it->second.end(); //Last transaction id
        auto diff_set_it = singleton->diff_set.begin();

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
                ++s_it;
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
        cout << endl << "Transactions for: " << it->first << endl;
        for(auto s_it = it->second.begin(); s_it != it->second.end(); ++s_it) {
            cout << *s_it << ", ";
        }
        cout << endl << "diff_set: " << endl;
        for(auto d_it = singleton->diff_set.begin(); d_it != singleton->diff_set.end(); ++d_it) {
            cout << *d_it << ", ";
        }
        cout << endl;

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

//Można poprawić uwzględniając min_sup
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
        (*it)->print();
        auto brother = it;
        brother++;
        for(auto b_it = brother; b_it != pNode->children.end(); ++b_it) {
            //Go deeper and create and calculate next level
            node *new_node;
            new_node = new node;
            //D(ab) = D(b) - D(a)
            //D(ab) = T(a) - T(b)
            //I have diff list, so
            difference((*b_it)->diff_set, (*it)->diff_set, new_node->diff_set);
            //if sup > minSup add to tree
            //Calculate sup
            auto sup = (*it)->support - new_node->diff_set.size();
            if(sup > min_sup) {
                new_node->element = (*b_it)->element;
                new_node->support = sup;
                tree.add(new_node, *it);
                cout << "Added: " << new_node->element << " at level: " << new_node->level << " to "
                     << new_node->parent->element << " from level: " << new_node->parent->level << endl;
            } else {
                delete new_node;
            }
        }
        traverse((*it));
    }
}

int main(int argc, const char **argv)
{
    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
            ("help,h", "produce help message")
            ("min_sup,m", po::value<int>(), "minimum support")
            ("data_set,d", po::value<std::string>(), "data set file")
            ("taxonomy,t", po::value<std::string>(), "taxonomy filename")
            ("out,o", po::value<std::string>(&out_filename)->default_value("out.txt"), "out filename")
            ("stat,s", po::value<std::string>(&stat_filename)->default_value("stat.txt"), "stat filename");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if(vm.count("help")) {
        cout << desc << "\n";
        return 1;
    }

    if(vm.count("min_sup")) {
        min_sup = vm["min_sup"].as<int>();
        cout << "Minimum support level was set to "
             << min_sup << ".\n";
    } else {
        cout << "Minimum support was not set. Provide as an argument --min_sup=2\n";
        return 1;
    }

    if(vm.count("data_set")) {
        data_filename = vm["data_set"].as<string>();
        cout << "data_set was set to "
             << data_filename << ".\n";
    } else {
        cout << "Argument data_set was not provided. Provide as filename to data --data_set=data.txt\n";
        return 1;
    }

    bool use_taxonomy;
    if(vm.count("taxonomy")) {
        taxonomy_filename = vm["taxonomy"].as<string>();
        cout << "taxonomy was set to "
             << taxonomy_filename << ".\n";
        auto s = get_wall_time();
        //cout<<"reading the hierarchy datasets start: "<<s<<endl;
        if(read_taxonomy(taxonomy_filename) < 0) {
            return 1;
        }
        auto e = get_wall_time();
        cout << "reading the hierarchy datasets: " << e - s << " sec." << endl;
        use_taxonomy = true;
    } else {
        use_taxonomy = false;
        cout
                << "Argument taxonomy was not provided. Calculation will be performed without hierarchy. To calculate dEclat with hierarchy, provide taxonomy filename --taxonomy=taxonomy_data.txt\n";
    }

    read_dataset(data_filename, use_taxonomy);

    //First level
    create_first_level_diff_sets();
    traverse(tree.root);
    cout << "------------" << endl;
    tree.print(tree.root);
    return 0;
}