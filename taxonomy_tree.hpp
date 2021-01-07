//
// Created by mic on 1/4/21.
//

#ifndef DECLAT_TAXONOMY_TREE_HPP
#define DECLAT_TAXONOMY_TREE_HPP

#include <vector>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <set>
#include <map>
#include "tree.hpp"

extern unsigned int number_of_frequent_itemsets;
extern unsigned int number_of_created_candidates;
extern std::map<unsigned int, std::pair<unsigned int, unsigned int>> number_of_created_candidates_and_frequent_itemsets_of_length; //For stats
extern struct tree tree;

struct taxonomy_node
{
    taxonomy_node *parent = nullptr; //Pointer to parent
    std::vector<taxonomy_node *> children; //List of children
    unsigned int element = 0; //Item id
    unsigned int support = 0; //Support number
    //Just for checking if everything is OK. To remove later on
    std::set<unsigned int> transaction_ids; //List of transaction ids for this node from vertical_representation. For parent it is sum of children's transaction_ids
    /**
     * @brief For leafs it is copied from tree diff_set is calculated in function create_first_level_diff_sets()
     * If parent has one child it is copied from child if for parent has more than one child it is common part of children diff_sets
     */
    std::set<unsigned int> diff_set; //set of difflist transactions ids
    //std::string name;

    /**
     * @brief merges this->transaction_ids with given with_set
     * @param with_set
     */
    void merge_transaction_ids(const std::set<unsigned int> &with_set);

    /**
     * @brief Calculates diff_set from children[].diff_set
     * @note diff_set for children MUST be already calculated
    */
    void calculate_diff_set_from_children();

    /**
     * @brief Just prints out node
     */
    void print();

    void clear_sets()
    {
        transaction_ids.clear();
        diff_set.clear();
    }

    /**
     * @brief Constructor
     * @note Does NOT add created element to parent.children
     * @param el - Element id
     * @param p - Parent node.
     */
    taxonomy_node(unsigned int el, taxonomy_node *p) : element(el), parent(p)
    {
    }

    ~taxonomy_node()
    {
        for(auto it = children.begin(); it != children.end(); ++it) {
            delete *it;
        }
    }

};

struct taxonomy_tree
{
    /**
     * @brief Tree may have many roots. If so than we have many trees in fact.
     * Roots are stored in roots variable. parent node is nullptr
     */
    std::unordered_map<unsigned int, taxonomy_node *> roots; //Needed for constructing tree from flat (node,parent) list

    /**
     * @brief vertical representation created from taxonomy_tree All except leafs
     */
    std::unordered_map<unsigned int, std::set<unsigned int> *> *tree_vertical_representation = nullptr;

    /**
     * @brief constructs tree_vertical_representation property based on tree nodes except leafs
     * This will be later on will be passed to dEclat algorithm
     */
    void create_vertical_representation();

    /**
     * @brief adds current_element to tree making its parent parent_element.
     * If tree is empty two nodes are created current_node and parent_node. Then this->roots[parent_element] = parent_node; is set
     * If tree is not empty then search in roots is performed and element is added to tree. roots may change
     *
     * @param current_element
     * @param parent_element
     */
    void add(unsigned int current_element, unsigned int parent_element);

    /**
     * @brief Method for searching element in tree
     * @param element Element which is we are looking for
     * @return pointer to taxonomy_node nullptr if not found
     */
    taxonomy_node *find(unsigned int element);

    taxonomy_node *find(taxonomy_node *pNode, unsigned int element, bool &found);

    /**
     * @brief prints tree elements
     */
    void print();

    /**
     * @brief Calculates support for taxonomy tree. If element has support > min_support than a parent of element is in taxonomy tree and also all parents are in tree and their support is greater than min_sup
     * Calls set_sup_from_vertical_representation() and set_sup_from_children()
     */
    void calculate_support(std::unordered_map<unsigned int, std::set<unsigned int> *> *vertical_representation);

    /**
     * Prints to file frequent (in fact all but leafs) elements from taxonomy tree
     * @param file Filename where to store data
     */
    void print_frequent_itemset(const std::string &file);

    void clear_sets_in_nodes();

    ~taxonomy_tree()
    {
        for(auto it = roots.begin(); it != roots.end(); ++it) {
            delete (*it).second;
        }

        if(tree_vertical_representation) {
            delete tree_vertical_representation;
        }
    }

private:
    /**
     * @brief Sets support property for tree node (taxonomy_node) which are not leafs - nodes that have children based on vertical_representation variable.
     * Also sets transaction_ids (copy) from vertical_representation.
     * In vertical_representation variable we have except items also first level of parent of item
     */
    void set_sup_from_vertical_representation(
            std::unordered_map<unsigned int, std::set<unsigned int> *> *vertical_representation);

    void set_sup_from_vertical_representation(taxonomy_node *pNode,
                                              std::unordered_map<unsigned int, std::set<unsigned int> *> *vertical_representation);

    void create_vertical_representation(taxonomy_node *pNode);
    /**
     * @brief sets support and transaction_ids property for tree nodes based on transaction_ids and support property from node's children.
     * transaction_ids are merged transaction_ids from children
     */
    void set_sup_from_children();

    void set_sup_from_children(taxonomy_node *pNode);

    void print_frequent_itemset(taxonomy_node *pNode, std::string all_ascendats, std::ofstream &file, int level);

    void print(taxonomy_node *pNode);

    void clear_sets_in_nodes(taxonomy_node *pNode);

};


#endif //DECLAT_TAXONOMY_TREE_HPP
