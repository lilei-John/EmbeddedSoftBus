#ifndef RBTREE_H
#define RBTREE_H

#include <stddef.h>

#define RB_RED      0
#define RB_BLACK    1

struct rb_node {
    unsigned long rb_parent_color;
    struct rb_node *rb_right;
    struct rb_node *rb_left;
};

struct rb_root {
    struct rb_node *rb_node;
};

#define RB_ROOT (struct rb_root) { NULL, }

#define rb_parent(r)   ((struct rb_node *)((r)->rb_parent_color & ~3))
#define rb_color(r)    ((r)->rb_parent_color & 1)
#define rb_is_red(r)   (!rb_color(r))
#define rb_is_black(r) rb_color(r)
#define rb_set_red(r)  do { (r)->rb_parent_color &= ~1; } while (0)
#define rb_set_black(r)  do { (r)->rb_parent_color |= 1; } while (0)
#define rb_set_color(r, c) do { \
    (r)->rb_parent_color = ((r)->rb_parent_color & ~1) | (c); \
} while (0)

#define rb_entry(ptr, type, member) container_of(ptr, type, member)
#define container_of(ptr, type, member) ({          \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type,member) );})

void rb_set_parent(struct rb_node *rb, struct rb_node *p);
void rb_link_node(struct rb_node *node, struct rb_node *parent,
                 struct rb_node **rb_link);
void rb_insert_color(struct rb_node *, struct rb_root *);
void rb_erase(struct rb_node *, struct rb_root *);
struct rb_node *rb_first(const struct rb_root *);
struct rb_node *rb_next(const struct rb_node *);

#endif // RBTREE_H 