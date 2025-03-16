#include <stdio.h>
#include <stdlib.h>


#define DECLARE_TREE(typename, type, arity) \
    typedef struct typename##Node { \
        type value; \
        struct typename##Node *children[arity];\
    } \
    typename##Node; \
    \
    typename##Node *typename##_new(type default_value) \
    { \
        typename##Node *node = (typename##Node *)malloc(sizeof(typename##Node)); \
        node->value = default_value; \
        for (int i = 0; i < arity; ++i) { \
            node->children[i] = NULL; \
        } \
        return node; \
    }\
    voide typename##_insert(typename##Node node, int *path, int path_length, type value, type default_value) { \
        typename##Node *current = node; \
        for (int i = 0; i < path_length; ++i) { \
            int index = path[i]; \
            if (current->children[index] == NULL) { \
                current->children[index] = typename##_new(default_value); \
            } \
            current = current->children[index]; \
        } \
        current->value = value; \
    } \
    void typename##_traverse(typename##Node *node, void (*func)(type))         \
    { \
        if (node == NULL)\
            return; \                                                    
        func(node->value); \                                                 
        for (int i = 0; i < arity; i++) {         \                             
            typename##_traverse(node->children[i], func);    \                  
        }           \                                                           
    }\







DECLARE_TREE(MyTree, double, 50)

int main(void) {
	MyTreeNode tree_node = MyTreeNode_new(0.25);
	MyTreeNode_insert(tree_node, {1, 2, 43, 37}, 1.37, 3.14);
	MyTreeNode_traverse(tree_node, my_printf);
	MyTreeNode_free(tree_node)
}
