#include "max_heap.h"

/* Returns 1 if node at index a has strictly higher priority than
 * node at index b, using the documented tie-break order. */
static int higher_priority(const HeapNode *a, const HeapNode *b) {
    if (a->saturation != b->saturation) return a->saturation > b->saturation;
    if (a->degree != b->degree) return a->degree > b->degree;
    return a->enrollment > b->enrollment;
}

static void swap_nodes(MaxHeap *h, int i, int j) {
    HeapNode tmp = h->data[i];
    h->data[i] = h->data[j];
    h->data[j] = tmp;
    h->position[h->data[i].id] = i;
    h->position[h->data[j].id] = j;
}

static void sift_up(MaxHeap *h, int i) {
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (higher_priority(&h->data[i], &h->data[parent])) {
            swap_nodes(h, i, parent);
            i = parent;
        } else {
            break;
        }
    }
}

static void sift_down(MaxHeap *h, int i) {
    for (;;) {
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        int largest = i;
        if (left < h->size && higher_priority(&h->data[left], &h->data[largest])) {
            largest = left;
        }
        if (right < h->size && higher_priority(&h->data[right], &h->data[largest])) {
            largest = right;
        }
        if (largest == i) break;
        swap_nodes(h, i, largest);
        i = largest;
    }
}

void heap_init(MaxHeap *h) {
    h->size = 0;
    for (int i = 0; i < MAX_SUBJECTS; i++) h->position[i] = -1;
}

void heap_insert(MaxHeap *h, int id, int saturation, int degree, int enrollment) {
    int i = h->size++;
    h->data[i].id = id;
    h->data[i].saturation = saturation;
    h->data[i].degree = degree;
    h->data[i].enrollment = enrollment;
    h->position[id] = i;
    sift_up(h, i);
}

HeapNode heap_extract_max(MaxHeap *h) {
    HeapNode top = h->data[0];
    h->position[top.id] = -1;
    h->size--;
    if (h->size > 0) {
        h->data[0] = h->data[h->size];
        h->position[h->data[0].id] = 0;
        sift_down(h, 0);
    }
    return top;
}

HeapNode heap_peek(const MaxHeap *h) {
    return h->data[0];
}

int heap_is_empty(const MaxHeap *h) {
    return h->size == 0;
}

void heap_update_saturation(MaxHeap *h, int id, int new_saturation) {
    int i = h->position[id];
    if (i < 0) return; /* not in heap (already colored/extracted) */
    int old = h->data[i].saturation;
    h->data[i].saturation = new_saturation;
    if (new_saturation > old) {
        sift_up(h, i);
    } else if (new_saturation < old) {
        sift_down(h, i);
    }
}

void heap_build(MaxHeap *h, const HeapNode *nodes, int n) {
    h->size = n;
    for (int i = 0; i < n; i++) {
        h->data[i] = nodes[i];
        h->position[nodes[i].id] = i;
    }
    /* Bottom-up heapify: O(n) total, not O(n log n). */
    for (int i = n / 2 - 1; i >= 0; i--) {
        sift_down(h, i);
    }
}
