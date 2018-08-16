#ifndef RADIX_TREE_NODE_HPP
#define RADIX_TREE_NODE_HPP

#include <map>
#include <functional>

template <typename K, typename T, typename Compare>
class radix_tree_node {
    friend class radix_tree<K, T, Compare>;
    friend class radix_tree_it<K, T, Compare>;

    typedef std::pair<const K, T> value_type;
    typedef typename std::map<K, radix_tree_node<K, T, Compare>*, Compare >::iterator it_child;

private:
	radix_tree_node(Compare& pred) : m_children(std::map<K, radix_tree_node<K, T, Compare>*, Compare>(pred)), m_parent(NULL), m_value(NULL), m_depth(0), m_is_leaf(false), m_key(), m_pred(pred) { }
    radix_tree_node(const value_type &val, Compare& pred);
    radix_tree_node(const radix_tree_node&); // delete
    radix_tree_node& operator=(const radix_tree_node&); // delete

    ~radix_tree_node();

    std::map<K, radix_tree_node<K, T, Compare>*, Compare> m_children;
    radix_tree_node<K, T, Compare> *m_parent;
    value_type *m_value;
    int m_depth;
    bool m_is_leaf;
    K m_key;
	Compare& m_pred;
};

template <typename K, typename T, typename Compare>
radix_tree_node<K, T, Compare>::radix_tree_node(const value_type &val, Compare& pred) :
    m_children(std::map<K, radix_tree_node<K, T, Compare>*, Compare>(pred)),
    m_parent(NULL),
    m_value(NULL),
    m_depth(0),
    m_is_leaf(false),
    m_key(), 
	m_pred(pred)
{
    m_value = new value_type(val);
}

template <typename K, typename T, typename Compare>
radix_tree_node<K, T, Compare>::~radix_tree_node()
{
    it_child it;
    for (it = m_children.begin(); it != m_children.end(); ++it) {
        delete it->second;
    }
    delete m_value;
}


#endif // RADIX_TREE_NODE_HPP

