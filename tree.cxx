//
// Created by mic on 12/20/20.
//

#include "tree.hpp"
#include <map>

using namespace std;

extern bool are_items_mapped_to_string;
extern std::map<unsigned int, string> names_for_items;

void tree::add(node *current_node, node *parent_node)
{
    current_node->parent = parent_node;
    if(parent_node != nullptr) {
        parent_node->children.push_back(current_node);
        current_node->level = parent_node->level + 1;
        if(current_node->level > this->max_level) {
            this->max_level = current_node->level;
        }
    }
}

void tree::print(node *pNode)
{
    //visited[pNode] = true;
    for(auto it = pNode->children.begin(); it != pNode->children.end(); ++it) {
        (*it)->print();
        //  auto search = visited.find(*it);
        //if(search == visited.end()) {
            this->print((*it));
        //}
    }
}


void tree::print(bool sorted)
{
    //visited.clear();
    this->print(root);
}

void tree::print_frequent_itemset(const std::string &file, bool sorted)
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
    if(sorted) {
        std::set<unsigned int> set_ascendant;
        this->print_frequent_itemset_sorted(this->root, set_ascendant, myfile);
    } else {
        this->print_frequent_itemset(this->root, ascendant, myfile);
    }
    if(myfile.is_open()) {
        myfile.close();
    }
}

void tree::print_frequent_itemset(node *pNode, std::string all_ascendants, ofstream &file)
{
    int el = pNode->element;
    for(auto it = pNode->children.begin(); it != pNode->children.end(); ++it) {
        ////format: length, sup, discovered_frequent_itemset
        string parent = all_ascendants;
        string str_element;
        if(are_items_mapped_to_string) {
            auto search = names_for_items.find((*it)->element);
            if(search != names_for_items.end()) {
                str_element = search->second;
            } else {
                str_element = to_string((*it)->element);
            }
        } else {
            str_element = to_string((*it)->element);
        }
        if((*it)->level > 1) {
            parent += ", ";
        }
        parent += str_element;
        if(file.is_open()) {
            file << (*it)->level << "\t" << (*it)->support << "\t" << parent << "" << endl;
        } else {
            cout << (*it)->level << "\t" << (*it)->support << "\t" << parent << "" << endl;
        }
        this->print_frequent_itemset((*it), parent, file);
    }
}

void tree::print_frequent_itemset_sorted(node *pNode, std::set<unsigned int> all_ascendants, ofstream &file)
{
    for(auto it = pNode->children.begin(); it != pNode->children.end(); ++it) {
        ////format: length, sup, discovered_frequent_itemset
        std::set<unsigned int> ascendants;
        ascendants = all_ascendants;
        ascendants.insert((*it)->element);
        string parent = "";
        if(are_items_mapped_to_string) {
            std::set<string> str_ascendants;
            for(auto p_it = ascendants.begin(); p_it != ascendants.end(); ++p_it) {
                auto search = names_for_items.find(*p_it);
                if(search != names_for_items.end()) {
                    str_ascendants.insert(search->second);
                } else {
                    str_ascendants.insert(to_string(*p_it));
                }
            }
            for(auto p_it = str_ascendants.begin(); p_it != str_ascendants.end(); ++p_it) {
                if(parent != "") {
                    parent += ", ";
                }
                parent += (*p_it);
            }
        } else {
            for(auto p_it = ascendants.begin(); p_it != ascendants.end(); ++p_it) {
                if(parent != "") {
                    parent += ", ";
                }
                parent += to_string((*p_it));
            }
        }
        if(file.is_open()) {
            file << (*it)->level << "\t" << (*it)->support << "\t" << parent << endl;
        } else {
            cout << (*it)->level << "\t" << (*it)->support << "\t" << parent << endl;
        }
        this->print_frequent_itemset_sorted((*it), ascendants, file);
    }
}

void node::print()
{
    cout << this << " Level: " << this->level << " element: " << this->element << " parent: "
         << (this->parent == nullptr ? 0 : this->parent->element) << " [" << this->parent << "]" << " sup: "
         << this->support << endl << "D({";
    for(auto it = this->diff_set.begin(); it != this->diff_set.end(); ++it) {
        cout << *it << ", ";
    }
    cout << "})" << endl;
}
