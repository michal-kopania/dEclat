//
// Created by mic on 1/4/21.
//

#include "taxonomy_tree.hpp"

using namespace std;


void taxonomy_tree::add(unsigned int current_element, unsigned int parent_element)
{
    taxonomy_node *current_node, *parent_node;

    //Search for parent_element in roots
    auto search = roots.find(parent_element);
    if(search != roots.end()) {
        //Found in roots. Add to found root
        parent_node = search->second;
        auto search_current = roots.find(current_element);
        if(search_current != roots.end()) {
            //Current is in roots.
            parent_node->children.push_back(search_current->second);
            for(auto it = parent_node->children.begin(); it != parent_node->children.end(); ++it) {
                (*it)->parent = parent_node;
            }
            this->roots[current_element] = nullptr;
            roots.erase(search_current);
        } else {
            current_node = new taxonomy_node(current_element, parent_node);
            parent_node->children.push_back(current_node);
        }
    } else {
        //Parent is not in roots
        //Is my current_node in roots?
        auto search_current = roots.find(current_element);
        if(search_current != roots.end()) {
            //Yes.
            parent_node = new taxonomy_node(parent_element, nullptr);
            parent_node->children.push_back(search_current->second);
            for(auto it = parent_node->children.begin(); it != parent_node->children.end(); ++it) {
                (*it)->parent = parent_node;
            }
            //Add parent to roots
            this->roots[parent_element] = parent_node;
            //Remove from roots. Must assign null first
            this->roots[current_element] = nullptr;
            roots.erase(search_current);
            //auto e=roots.extract(search_current);

        } else {
            //Add pair current,parent add parent to roots
            parent_node = new taxonomy_node(parent_element, nullptr);
            current_node = new taxonomy_node(current_element, parent_node);
            parent_node->children.push_back(current_node);
            this->roots[parent_element] = parent_node;
        }
    }
}

void taxonomy_tree::print()
{
    for(auto it = roots.begin(); it != roots.end(); ++it) {
        print((*it).second);
    }
}

void taxonomy_tree::print(taxonomy_node *pNode)
{
    pNode->print();
    for(auto it = pNode->children.begin(); it != pNode->children.end(); ++it) {
        if((*it) != nullptr) {
            this->print((*it));
        }
    }
}


void taxonomy_tree::set_sup_from_vertical_representation()
{
    for(auto it = roots.begin(); it != roots.end(); ++it) {
        set_sup_from_vertical_representation((*it).second);
    }
}

void taxonomy_tree::set_sup_from_vertical_representation(taxonomy_node *pNode)
{
    for(auto it = pNode->children.begin(); it != pNode->children.end(); ++it) {
        if((*it) != nullptr) {
            this->set_sup_from_vertical_representation((*it));
        }
    }
    if(!pNode->children.empty()) {
        auto search = vertical_representation->find(pNode->element);
        if(search != vertical_representation->end()) {
            pNode->support = search->second.size();
            pNode->transaction_ids = search->second; //copy. Its safer that way
        }
        //pNode->print();
    }
}

void taxonomy_tree::set_sup_from_children()
{
    for(auto it = roots.begin(); it != roots.end(); ++it) {
        set_sup_from_children((*it).second);
    }
}

void taxonomy_tree::set_sup_from_children(taxonomy_node *pNode)
{
    for(auto it = pNode->children.begin(); it != pNode->children.end(); ++it) {
        if((*it) != nullptr) {
            this->set_sup_from_children((*it));
            //Child has calculated sup
            if(!(*it)->transaction_ids.empty()) {
                //merge.
                pNode->merge_transaction_ids((*it)->transaction_ids);
                //pNode->transaction_ids.merge((*it)->transaction_ids);
                pNode->support = pNode->transaction_ids.size();
            }
        }

    }
//    if(!pNode->children.empty()){
//         pNode->print();
//    }
}

taxonomy_node *taxonomy_tree::find(taxonomy_node *pNode, unsigned int element, bool &found)
{
    taxonomy_node *found_node = nullptr;
    if(!found) {
        if(pNode->element == element) {
            found = true;
            found_node = pNode;
            return found_node;
        }
        for(auto it = pNode->children.begin(); it != pNode->children.end(); ++it) {
            this->find((*it), element, found);
        }
    }
    return found_node;
}

taxonomy_node *taxonomy_tree::find(unsigned int element)
{
    //Search for element
    bool found = false;
    taxonomy_node *found_node = nullptr;
    for(auto it = roots.begin(); it != roots.end(); ++it) {
        found_node = this->find((*it).second, element, found);
        if(found) {
            break;
        }
    }
    return found_node;
}

void taxonomy_node::print()
{
    cout << this << " element: " << this->element << " parent: "
         << (this->parent == nullptr ? 0 : this->parent->element) << " [" << this->parent << "]" << " sup: "
         << this->support << endl;
    for(auto it = this->transaction_ids.begin(); it != this->transaction_ids.end(); ++it) {
        cout << (*it) << ", ";
    }
    cout << endl;
}

void taxonomy_node::merge_transaction_ids(const std::set<unsigned int> &with_set)
{
    if(this->transaction_ids.empty()) {
        this->transaction_ids = with_set;
    } else {
        for(auto it = with_set.begin(); it != with_set.end(); ++it) {
            this->transaction_ids.insert((*it));
        }
    }
}

void taxonomy_tree::print_frequent_itemset(const std::string &file)
{
    string ascendant = "";
    ofstream myfile;
    if(file != "") {
        myfile.open(file, fstream::out | fstream::trunc);
        if(!myfile) {
            cout << "Cannot open file: " << file << endl << "Results will not be saved and be printed at stdio instead"
                 << endl;
            myfile.close();
        } else {
            myfile << "length\tsup\tdiscovered_frequent_itemset" << endl;
        }
    }
    for(auto it = roots.begin(); it != roots.end(); ++it) {
        this->print_frequent_itemset((*it).second, ascendant, myfile, 0);
    }
    if(myfile.is_open()) {
        myfile.close();
    }
}

void
taxonomy_tree::print_frequent_itemset(taxonomy_node *pNode, std::string all_ascendats, std::ofstream &file, int level)
{
    ////format: length, sup, discovered_frequent_itemset
    ++level;
    string parent = all_ascendats;
    if(pNode->children.empty()) {
        return;
    }
    //For STATS
    ++number_of_created_candidates;
    ++number_of_frequent_itemsets;
    auto search = number_of_created_candidates_and_frequent_itemsets_of_length.find(level);
    std::pair<std::map<unsigned int, pair<unsigned int, unsigned int>>::iterator, bool> inserted;
    if(search == number_of_created_candidates_and_frequent_itemsets_of_length.end()) {
        inserted = number_of_created_candidates_and_frequent_itemsets_of_length.emplace(level,
                                                                                        pair(1, 0));
        search = inserted.first;
    } else {
        (*search).second.first++; //Increase number of candidates
    }
    (*search).second.second++;
    //For STATS end

    if(level > 1) {
        parent += ", ";
    }

    parent += to_string(pNode->element);

    if(file.is_open()) {
        file << level << "\t" << pNode->support << "\t" << parent << "" << endl;
    } else {
        cout << level << "\t" << pNode->support << "\t" << parent << "" << endl;
    }
    for(auto it = pNode->children.begin(); it != pNode->children.end(); ++it) {
        this->print_frequent_itemset((*it), parent, file, level);
    }
}

void taxonomy_tree::calculate_support()
{
    this->set_sup_from_vertical_representation();
#if DEBUG_LEVEL > 0
    cout<<"------------- after set_sup_from_vertical_representation ------"<<endl;
    hierarchy_tree.print();
#endif
    this->set_sup_from_children();
#if DEBUG_LEVEL > 0
    cout<<"------------- after set_sup_from_children --------"<<endl;
    hierarchy_tree.print();
#endif
}

