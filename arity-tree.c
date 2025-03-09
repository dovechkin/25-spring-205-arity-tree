#include <stdio.h>
#include <stdlib.h>

#define DECLARE_TREE(typename, type, arity)                                    \
    typedef struct typename##Node                                              \
    {                                                                          \
        type value;                                                            \
        struct typename##Node *children[arity];                                \
    }                                                                          \
    typename##Node;                                                            \
                                                                               \
    typename##Node *typename##_new(type default_value)                         \
    {                                                                          \
        typename##Node *node =                                                 \
            (typename##Node *)malloc(sizeof(typename##Node));                  \
        node->value = default_value;                                           \
        for (int i = 0; i < arity; i++) {                                      \
            node->children[i] = NULL;                                          \
        }                                                                      \
        return node;                                                           \
    }                                                                          \
                                                                               \
    void typename##_insert(typename##Node *node, int *path, int path_length,   \
                           type value, type default_value)                     \
    {                                                                          \
        typename##Node *current = node;                                        \
        for (int i = 0; i < path_length - 1; i++) {                            \
            int index = path[i];                                               \
            if (current->children[index] == NULL) {                            \
                current->children[index] = typename##_new(default_value);      \
            }                                                                  \
            current = current->children[index];                                \
        }                                                                      \
        int last_index = path[path_length - 1];                                \
        if (current->children[last_index] != NULL) {                           \
            typename##Node *displaced_node = current->children[last_index];    \
            current->children[last_index] = typename##_new(value);             \
            current->children[last_index]->children[0] = displaced_node;       \
        } else {                                                               \
            current->children[last_index] = typename##_new(value);             \
        }                                                                      \
    }                                                                          \
                                                                               \
    void typename##_traverse(typename##Node *node, void (*func)(type))         \
    {                                                                          \
        if (node == NULL)                                                      \
            return;                                                            \
        func(node->value);                                                     \
        for (int i = 0; i < arity; i++) {                                      \
            typename##_traverse(node->children[i], func);                      \
        }                                                                      \
    }                                                                          \
                                                                               \
    void typename##_free(typename##Node *node)                                 \
    {                                                                          \
        if (node == NULL)                                                      \
            return;                                                            \
        for (int i = 0; i < arity; i++) {                                      \
            typename##_free(node->children[i]);                                \
        }                                                                      \
        free(node);                                                            \
    }

DECLARE_TREE(MyTree, double, 50)

void my_printf(double value)
{
    printf("%f ", value);
}

int main(void)
{
    MyTreeNode *tree_node = MyTree_new(0.25);
    int path[] = {1, 2, 43, 37};
    MyTree_insert(tree_node, path, 4, 1.37, 3.14);
    MyTree_traverse(tree_node, my_printf);
    MyTree_free(tree_node);
    return 0;
}