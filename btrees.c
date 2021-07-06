#include <stdio.h>
#include <stdlib.h>

struct bnode {
  int t; // the minimal degree (no of children)
  int n; // the number of keys
  int* key; // keys: 0...n-1
  struct bnode** c; // children: 0...n
  int leaf;
};

struct bnode* make_bnode(int t) {
  struct bnode* bn = malloc(sizeof(struct bnode));
  bn->t = t;
  bn->n = 0;
  bn->key = calloc(2*t-1, sizeof(int));
  bn->c = calloc(2*t, sizeof(struct bnode*));
  bn->leaf = 1;

  return bn;
}

void delete_bnode(struct bnode* bn) {
  if (bn == NULL) return;
  free(bn->key);
  for(int i = 0; i <= bn->n; ++i) {
    delete_bnode((bn->c)[i]);
  }
  free(bn->c);
  free(bn);
}

struct btree {
  struct bnode* root;
};

struct btree* make_btree(int t) {
  struct bnode* root = make_bnode(t);
  struct btree* tree = malloc(sizeof(struct btree));
  tree->root = root;
  return tree;
}

void delete_tree(struct btree* tree) {
  delete_bnode(tree->root);
  free(tree);
}

struct seekres {
  struct bnode* bn;
  int index;
};

struct seekres search_btree(struct bnode* bn, int key) {
  int interval_index = 0;
  while (interval_index < bn->n && bn->key[interval_index] < key) ++interval_index;

  struct seekres retval;
  retval.index = interval_index;

  if (interval_index < bn->n && key == bn->key[interval_index]) {
    retval.bn = bn;
    return retval;
  }
  if (bn->leaf) {
    retval.bn = NULL;
    return retval;
  } else {
    return search_btree(bn->c[interval_index], key);
  }

}

void split_child_btree(struct bnode* parent, int i, struct bnode* child) {
  // it is assumed that the child is full (it has 2*t children and 2*t-1 keys)
  // and the parent is not full

  int t = parent->t; // this is common to the entire tree

  struct bnode* nnode =  make_bnode(t);
  nnode->leaf = child->leaf;
  nnode->n = t - 1;

  // copy to nnode all keys from child with numbers t,...,2*t-2
  for (int j = 0; j < t - 1; ++j) nnode->key[j] = child->key[j+t];
  
  // copy to nnode all the links for the above
  if (!child->leaf) {
    for (int j = 0; j <= t - 1; ++j) {
      nnode->c[j] = child->c[j+t];
      child->c[j+t] = NULL;
    }
  } 
  child->n = t - 1;

  // move all the keys from the i-th position (which is doubled) to the right in the parent
  for (int j = parent->n-1; j >= i; --j) parent->key[j+1] = parent->key[j];
  parent->key[i] = child->key[t-1]; // add the new key that divided the child here

  // adjust the links for the parent 
  for (int j = parent->n; j > i; --j) parent->c[j+1] = parent->c[j]; // move right
  parent->c[i+1] = nnode;

  ++parent->n;
}

void insert_nonfull_btree(struct bnode* nonfull_node, int key){
  int t = nonfull_node->t; // this is common to the entire tree  
  int i = nonfull_node->n - 1;
  
  if (nonfull_node->leaf) { // insert the node into the leaf
    while (i >= 0 && nonfull_node->key[i] > key) {
      nonfull_node->key[i+1] = nonfull_node->key[i];
      --i;
    }
    nonfull_node->key[i+1] = key;
  ++(nonfull_node->n);
  } else { // find the place to insert the node...
    while (i >= 0 && key < nonfull_node->key[i]) --i;
    ++i;

    // ...and insert it below
    if (nonfull_node->c[i]->n == 2*t-1) { // ...splitting the child if needed
      split_child_btree(nonfull_node, i, nonfull_node->c[i]);
      if (key > nonfull_node->key[i]) ++i;
    }
    insert_nonfull_btree(nonfull_node->c[i], key);
    
  }

}

void insert_btree(struct btree* tree, int key) {
  struct bnode* r = tree->root;

  if (r->n == 2*r->t - 1) { // split 
    struct bnode* newroot = make_bnode(r->t);
    tree->root = newroot;
    newroot->leaf = 0;
    newroot->n = 0;
    newroot->c[0] = r;
    split_child_btree(newroot, 0, r);
    insert_nonfull_btree(newroot, key);
  } else 
    insert_nonfull_btree(r, key);
}

void print_aux_nname(struct bnode* bn) {
  printf("\"");
  for (int i = 0; i < bn->n; ++i) {
    printf("|%d", bn->key[i]);
  }
  printf("|\"");
}

void to_dot_rec(struct bnode* bn) {
  for (int i = 0; i <= bn->n; ++i) {
    if (bn->c[i] != NULL) {
      printf("\t");
      print_aux_nname(bn);
      printf(" -> ");
      print_aux_nname(bn->c[i]);
      printf(";\n");
    }
  }
  if (!bn->leaf)
  for (int i = 0; i <= bn->n; ++i) {
      to_dot_rec(bn->c[i]);
  }
}

void to_dot(struct bnode* bn) {
  printf("digraph tr {\n");
  printf("\tsize=\"6,6\";\n");
  printf("\tnode [color=lightblue2, style=filled];\n");
  if (bn->leaf) {
    printf("\t");
    print_aux_nname(bn);
    printf(";\n");
  } else to_dot_rec(bn);
  printf("}\n");
}

int main(int argc, char** argv) {
  // test: insert argv[1] nodes
  if (argc < 2) {
    printf("Usage: ./btrees no_of_inserted_nodes\n");
    printf("e.g., with graphviz you can do: './btrees 10 | dot -Tpdf -o d.pdf' to get a pic of 10-node B-tree in d.pdf\n");
    return 1;
  }
  
  int N = atoi(argv[1]);
  struct btree* tree = make_btree(2);
  for (int i = 0; i < N; ++i) insert_btree(tree, i);

  to_dot(tree->root);

  delete_tree(tree);
  return 0;
}
