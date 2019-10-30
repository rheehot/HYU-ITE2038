#include <stdlib.h>

#include "bpt.h"
#include "utility.h"
#include "test.h"

int bpt_test_preprocess(struct bpt_t* bpt,
                        struct file_manager_t* file,
                        struct buffer_manager_t* buffers)
{
    TEST_SUCCESS(buffer_manager_init(buffers, 4));
    TEST_SUCCESS(file_open(file, "testfile"));
    TEST_SUCCESS(bpt_init(bpt, file, buffers));
    return SUCCESS;
}

int bpt_test_postprocess(struct bpt_t* bpt,
                         struct file_manager_t* file,
                         struct buffer_manager_t* buffers)
{
    TEST_SUCCESS(bpt_release(bpt));
    TEST_SUCCESS(buffer_manager_shutdown(buffers));
    TEST_SUCCESS(file_close(file));
    remove("testfile");
    return SUCCESS;
}

TEST_SUITE(swap_ubuffer, {
    struct buffer_t buf1;
    struct buffer_t buf2;
    buf1.use_count = 10;
    buf2.use_count = 20;

    struct ubuffer_t ubuf1;
    struct ubuffer_t ubuf2;
    ubuf1.buf = &buf1;
    ubuf1.use_count = 10;
    ubuf2.buf = &buf2;
    ubuf2.use_count = 20;

    swap_ubuffer(&ubuf1, &ubuf2);
    TEST(ubuf1.use_count == 20);
    TEST(ubuf1.buf->use_count == 20);
    TEST(ubuf2.use_count == 10);
    TEST(ubuf2.buf->use_count == 10);
})

TEST_SUITE(bpt_buffering, {
    // just porting buffer_manager_buffering
})

TEST_SUITE(bpt_create_page, {
    // just porting buffer_manager_new_page
})

TEST_SUITE(bpt_free_page, {
    // just porting buffer_manager_free_page
})

TEST_SUITE(bpt_init, {
    struct bpt_t config;
    struct file_manager_t file;
    struct buffer_manager_t buffers;

    TEST_SUCCESS(bpt_init(&config, &file, &buffers));
    TEST(config.file == &file);
    TEST(config.buffers == &buffers);
    TEST(config.leaf_order == 32);
    TEST(config.internal_order == 249);
    TEST(config.verbose_output == FALSE);
    TEST(config.delayed_merge == TRUE);
})

TEST_SUITE(bpt_release, {
    struct bpt_t config;
    TEST_SUCCESS(bpt_release(&config));
    TEST(config.leaf_order == 0);
    TEST(config.internal_order == 0);
    TEST(config.verbose_output == FALSE);
    TEST(config.delayed_merge == FALSE);
    TEST(config.file == NULL);
    TEST(config.buffers == NULL);
})

TEST_SUITE(bpt_default_config, {
    struct bpt_t config;
    TEST_SUCCESS(bpt_default_config(&config));
    TEST(config.leaf_order == 32);
    TEST(config.internal_order == 249);
    TEST(config.verbose_output == FALSE);
    TEST(config.delayed_merge == TRUE);
})

TEST_SUITE(bpt_test_config, {
    struct bpt_t config;
    TEST_SUCCESS(bpt_test_config(&config, 5, 4));
    TEST(config.leaf_order == 5);
    TEST(config.internal_order == 4);
    TEST(config.verbose_output == TRUE);
    TEST(config.delayed_merge == TRUE);
})

TEST_SUITE(queue, {
    int i;
    struct queue_t* queue = NULL;
    for (i = 0; i < 10; ++i) {
        queue = enqueue(queue, i);
    }

    struct queue_t* tmp = queue;
    for (i = 0; i < 10; ++i) {
        TEST(tmp->pagenum == i);
        tmp = tmp->next;
    }

    pagenum_t n;
    for (i = 0; i < 5; ++i) {
        queue = dequeue(queue, &n);
        TEST(n == i);
    }

    for (i = 10; i < 15; ++i) {
        queue = enqueue(queue, i);
    }

    for (i = 5; i < 15; ++i) {
        queue = dequeue(queue, &n);
        TEST(n == i);
    }
})

TEST_SUITE(record_vec_init, {
    struct record_vec_t vec;
    TEST_SUCCESS(record_vec_init(&vec, 3));
    TEST(vec.size == 0);
    TEST(vec.capacity == 3);
    TEST(vec.rec != NULL);
    free(vec.rec);
})

TEST_SUITE(record_vec_free, {
    struct record_vec_t vec;
    TEST_SUCCESS(record_vec_init(&vec, 3));
    TEST_SUCCESS(record_vec_free(&vec));
    TEST(vec.size == 0);
    TEST(vec.capacity == 0);
    TEST(vec.rec == NULL);
})

TEST_SUITE(record_vec_expand, {
    struct record_vec_t vec;
    TEST_SUCCESS(record_vec_init(&vec, 5));

    int i;
    for (i = 0; i < 3; ++i) {
        vec.rec[i].key = 10 * i;
        vec.size++;
    }

    TEST_SUCCESS(record_vec_expand(&vec));
    TEST(vec.size == 3);
    TEST(vec.capacity == 10);
    
    for (i = 0; i < 3; ++i) {
        TEST(vec.rec[i].key == 10 * i);
    }

    TEST_SUCCESS(record_vec_free(&vec));
})

TEST_SUITE(record_vec_append, {
    struct record_vec_t vec;
    TEST_SUCCESS(record_vec_init(&vec, 5));

    int i;
    struct record_t rec;
    for (i = 0; i < 13; ++i) {
        rec.key = i * 10;
        TEST_SUCCESS(record_vec_append(&vec, &rec));
        TEST(vec.rec[i].key == i * 10);
    }

    TEST_SUCCESS(record_vec_free(&vec));
})

TEST_SUITE(height, {

})

TEST_SUITE(path_to_root, {

})

TEST_SUITE(cut, {
    TEST(cut(4) == 2);
    TEST(cut(5) == 3);
})

TEST_SUITE(find_leaf, {

})

TEST_SUITE(find_key_from_leaf, {
    // preproc
    int i;
    struct bpt_t bpt;
    struct file_manager_t file;
    struct buffer_manager_t buffers;
    TEST_SUCCESS(bpt_test_preprocess(&bpt, &file, &buffers));

    // case 0. leaf validation
    struct ubuffer_t node = make_node(&bpt, FALSE);
    struct page_t* page = from_ubuffer(&node);
    for (i = 0; i < 3; ++i) {
        page_header(page)->number_of_keys++;
        records(page)[i].key = i * 10;
    }

    TEST(find_key_from_leaf(10, node, NULL) == FAILURE);

    // case 1. cannot find
    page_header(page)->is_leaf = TRUE;
    records(page)[1].key = 15;
    TEST(find_key_from_leaf(10, node, NULL) == FAILURE);

    // case 2. find and record=NULL
    records(page)[1].key = 10;
    TEST_SUCCESS(find_key_from_leaf(10, node, NULL));

    records(page)[0].key = 10;
    records(page)[1].key = 15;
    TEST_SUCCESS(find_key_from_leaf(10, node, NULL));

    records(page)[0].key = 5;
    records(page)[1].key = 7;
    records(page)[2].key = 10;
    TEST_SUCCESS(find_key_from_leaf(10, node, NULL));

    // case 3. find and record
    struct record_t rec;
    *(int*)records(page)[2].value = 40;
    TEST_SUCCESS(find_key_from_leaf(10, node, &rec));
    TEST(rec.key == 10);
    TEST(*(int*)rec.value == 40);

    // postproc
    TEST_SUCCESS(bpt_test_postprocess(&bpt, &file, &buffers));
})

TEST_SUITE(bpt_find, {

})

TEST_SUITE(bpt_find_range, {

})

TEST_SUITE(print_leaves, {
    // print method
})

TEST_SUITE(print_tree, {
    // print method
})

TEST_SUITE(find_and_print, {
    // print method
})

TEST_SUITE(find_and_print_range, {
    // print method
})

TEST_SUITE(make_record, {
    struct record_t rec;

    int value = 100;
    TEST_SUCCESS(make_record(&rec, 10, (uint8_t*)&value, sizeof(int)));
    TEST(rec.key == 10);
    TEST(*(int*)rec.value == value);

    int i;
    int array[200];
    for (i = 0; i < 200; ++i) {
        array[i] = i;
    }

    TEST_SUCCESS(make_record(&rec, 20, (const uint8_t*)array, sizeof(int) * 200));
    TEST(rec.key == 20);

    int* ptr = (int*)rec.value;
    for (i = 0; i < 30; ++i) {
        TEST(ptr[i] == array[i]);
    }
})

TEST_SUITE(make_node, {
    struct bpt_t bpt;
    struct file_manager_t file;
    struct buffer_manager_t buffers;
    TEST_SUCCESS(bpt_test_preprocess(&bpt, &file, &buffers));

    struct ubuffer_t buf = make_node(&bpt, TRUE);
    struct page_header_t* header = page_header(from_ubuffer(&buf));
    TEST(header->parent_page_number == INVALID_PAGENUM);
    TEST(header->is_leaf == TRUE);
    TEST(header->number_of_keys == 0);
    TEST(header->special_page_number == INVALID_PAGENUM);

    TEST_SUCCESS(bpt_test_postprocess(&bpt, &file, &buffers));
})

TEST_SUITE(get_index, {
    struct bpt_t bpt;
    struct file_manager_t file;
    struct buffer_manager_t buffers;
    TEST_SUCCESS(bpt_test_preprocess(&bpt, &file, &buffers));

    struct ubuffer_t buf = make_node(&bpt, FALSE);
    struct page_header_t* header = page_header(from_ubuffer(&buf));
    header->number_of_keys = min(5, bpt.internal_order - 1);
    header->special_page_number = 10;

    int i;
    for (i = 0; i < header->number_of_keys; ++i) {
        entries(from_ubuffer(&buf))[i].pagenum = (i + 2) * 10;
    }

    TEST(get_index(&buf, 5) == header->number_of_keys);
    TEST(get_index(&buf, 10 * (header->number_of_keys + 3))
        == header->number_of_keys);

    TEST(get_index(&buf, 10) == -1);
    for (i = 0; i < header->number_of_keys; ++i) {
        TEST(get_index(&buf, (i + 2) * 10) == i);
    }

    TEST_SUCCESS(bpt_test_postprocess(&bpt, &file, &buffers));
})

TEST_SUITE(insert_into_leaf, {
    struct bpt_t bpt;
    struct file_manager_t file;
    struct buffer_manager_t buffers;
    TEST_SUCCESS(bpt_test_preprocess(&bpt, &file, &buffers));

    TEST_SUCCESS(bpt_test_config(&bpt, 5, 5));
    struct ubuffer_t buf = make_node(&bpt, TRUE);

    // case 0. ordered input
    int i;
    struct record_t rec;
    for (i = 0; i < 5; ++i) {
        rec.key = i;
        TEST_SUCCESS(insert_into_leaf(&bpt, &buf, &rec));
    }

    for (i = 0; i < 5; ++i) {
        TEST(records(from_ubuffer(&buf))[i].key == i);
    }

    // case 0.1. overflow
    TEST(insert_into_leaf(&bpt, &buf, &rec) == FAILURE);

    // case 1. reversed ordered input
    page_header(from_ubuffer(&buf))->number_of_keys = 0;
    for (i = 0; i < 5; ++i) {
        rec.key = 5 - i;
        TEST_SUCCESS(insert_into_leaf(&bpt, &buf, &rec));
    }

    for (i = 0; i < 5; ++i) {
        TEST(records(from_ubuffer(&buf))[i].key == i + 1);
    }

    // case 2. random input
    int arr[5];
    arr[0] = 1;
    arr[1] = 3;
    arr[2] = 2;
    arr[3] = 5;
    arr[4] = 4;

    page_header(from_ubuffer(&buf))->number_of_keys = 0;
    for (i = 0; i < 5; ++i) {
        rec.key = arr[i];
        TEST_SUCCESS(insert_into_leaf(&bpt, &buf, &rec));
    }

    for (i = 0; i < 5; ++i) {
        TEST(records(from_ubuffer(&buf))[i].key == i + 1);
    }

    TEST_SUCCESS(bpt_test_postprocess(&bpt, &file, &buffers));
})

TEST_SUITE(insert_into_leaf_after_splitting, {

})

TEST_SUITE(insert_into_node, {
    struct bpt_t bpt;
    struct file_manager_t file;
    struct buffer_manager_t buffers;
    TEST_SUCCESS(bpt_test_preprocess(&bpt, &file, &buffers));

    TEST_SUCCESS(bpt_test_config(&bpt, 5, 5));
    struct ubuffer_t buf = make_node(&bpt, FALSE);

    // case 0. ordered input
    int i;
    struct internal_t ent;
    for (i = 0; i < 5; ++i) {
        ent.key = i;
        TEST_SUCCESS(insert_into_node(&bpt, &buf, i, &ent));
    }

    for (i = 0; i < 5; ++i) {
        TEST(entries(from_ubuffer(&buf))[i].key == i);
    }

    // case 0.1. overflow
    TEST(insert_into_node(&bpt, &buf, 5, &ent) == FAILURE);

    // // case 1. reversed ordered input
    page_header(from_ubuffer(&buf))->number_of_keys = 0;
    for (i = 0; i < 5; ++i) {
        ent.key = 5 - i;
        TEST_SUCCESS(insert_into_node(&bpt, &buf, 0, &ent));
    }

    for (i = 0; i < 5; ++i) {
        TEST(entries(from_ubuffer(&buf))[i].key == i + 1);
    }

    TEST_SUCCESS(bpt_test_postprocess(&bpt, &file, &buffers));

})

TEST_SUITE(insert_into_node_after_splitting, {

})

TEST_SUITE(insert_into_parent, {
    int i;
    struct bpt_t bpt;
    struct file_manager_t file;
    struct buffer_manager_t buffers;
    TEST_SUCCESS(bpt_test_preprocess(&bpt, &file, &buffers));
    TEST_SUCCESS(bpt_test_config(&bpt, 7, 5));

    // case 0. new root
    struct ubuffer_t left = make_node(&bpt, TRUE);
    struct ubuffer_t right = make_node(&bpt, TRUE);

    BUFFER(left, WRITE_FLAG, {
        page_header(from_ubuffer(&left))->parent_page_number = INVALID_PAGENUM;
    })
    TEST_SUCCESS(insert_into_parent(&bpt, &left, 10, &right));

    struct ubuffer_t buf = bpt_buffering(&bpt, FILE_HEADER_PAGENUM);
    pagenum_t root = file_header(from_ubuffer(&buf))->root_page_number;

    buf = bpt_buffering(&bpt, root);
    struct page_t* page = from_ubuffer(&buf);
    TEST(page_header(page)->is_leaf == FALSE);
    TEST(page_header(page)->number_of_keys == 1);
    TEST(page_header(page)->parent_page_number == INVALID_PAGENUM);
    TEST(page_header(page)->special_page_number == left.buf->pagenum);
    TEST(entries(page)[0].pagenum == right.buf->pagenum);
    TEST(entries(page)[0].key == 10);

    // case 1. simple insert
    struct ubuffer_t parent = make_node(&bpt, FALSE);
    struct ubuffer_t temporal = make_node(&bpt, TRUE);
    BUFFER(parent, WRITE_FLAG, {
        page = from_ubuffer(&parent);
        page_header(page)->number_of_keys = 1;
        page_header(page)->special_page_number = temporal.buf->pagenum;
        entries(page)[0].key = 10;
        entries(page)[0].pagenum = left.buf->pagenum;
    })
    BUFFER(temporal, WRITE_FLAG, {
        page_header(from_ubuffer(&temporal))->special_page_number = left.buf->pagenum;
    })
    BUFFER(left, WRITE_FLAG, {
        page_header(from_ubuffer(&left))->special_page_number = INVALID_PAGENUM;
        page_header(from_ubuffer(&left))->parent_page_number = parent.buf->pagenum;    
        for (i = 0; i < 3; ++i) {
            records(from_ubuffer(&left))[i].key = 10 + i;
            page_header(from_ubuffer(&left))->number_of_keys++;
        }
    })
    BUFFER(right, WRITE_FLAG, {
        page_header(from_ubuffer(&right))->parent_page_number = parent.buf->pagenum;
        for (i = 0; i < 3; ++i) {
            records(from_ubuffer(&right))[i].key = 13 + i;
            page_header(from_ubuffer(&right))->number_of_keys++;
        }
    })

    TEST_SUCCESS(insert_into_parent(&bpt, &left, 13, &right));

    page = from_ubuffer(&parent);
    TEST(page_header(page)->number_of_keys == 2);
    TEST(page_header(page)->special_page_number == left.buf->pagenum);
    TEST(entries(page)[0].key == 10);
    TEST(entries(page)[0].pagenum == left.buf->pagenum);
    TEST(entries(page)[1].key == 13);
    TEST(entries(page)[1].pagenum == right.buf->pagenum);
    TEST(page_header(from_ubuffer(&temporal))->special_page_number == left.buf->pagenum);
    TEST(page_header(from_ubuffer(&left))->special_page_number == right.buf->pagenum);
    TEST(page_header(from_ubuffer(&right))->special_page_number == INVALID_PAGENUM);

    // case 2. node split

    TEST_SUCCESS(bpt_test_postprocess(&bpt, &file, &buffers));
})

TEST_SUITE(insert_into_new_root, {
    struct bpt_t bpt;
    struct file_manager_t file;
    struct buffer_manager_t buffers;
    TEST_SUCCESS(bpt_test_preprocess(&bpt, &file, &buffers));

    struct ubuffer_t left = make_node(&bpt, TRUE);
    struct ubuffer_t right = make_node(&bpt, TRUE);

    TEST_SUCCESS(insert_into_new_root(&bpt, &left, 10, &right));
    
    struct ubuffer_t buf = bpt_buffering(&bpt, FILE_HEADER_PAGENUM);
    pagenum_t root = file_header(from_ubuffer(&buf))->root_page_number;

    buf = bpt_buffering(&bpt, root);
    struct page_t* page = from_ubuffer(&buf);
    TEST(page_header(page)->is_leaf == FALSE);
    TEST(page_header(page)->number_of_keys == 1);
    TEST(page_header(page)->parent_page_number == INVALID_PAGENUM);
    
    TEST(page_header(page)->special_page_number == left.buf->pagenum);
    TEST(entries(page)[0].pagenum == right.buf->pagenum);

    TEST(page_header(from_ubuffer(&left))->parent_page_number == buf.buf->pagenum);
    TEST(page_header(from_ubuffer(&right))->parent_page_number == buf.buf->pagenum);

    TEST_SUCCESS(bpt_test_postprocess(&bpt, &file, &buffers));
})

TEST_SUITE(start_new_tree, {
    struct bpt_t bpt;
    struct file_manager_t file;
    struct buffer_manager_t buffers;
    TEST_SUCCESS(bpt_test_preprocess(&bpt, &file, &buffers));

    struct record_t rec;
    rec.key = 10;
    *(int*)rec.value = 20;

    TEST_SUCCESS(start_new_tree(&bpt, &rec));

    struct ubuffer_t ubuf = bpt_buffering(&bpt, FILE_HEADER_PAGENUM);
    pagenum_t root = file_header(from_ubuffer(&ubuf))->root_page_number;
    TEST(root != INVALID_PAGENUM);

    ubuf = bpt_buffering(&bpt, root);
    struct page_t* page = from_ubuffer(&ubuf);
    TEST(page_header(page)->is_leaf == TRUE);
    TEST(page_header(page)->number_of_keys == 1);
    TEST(page_header(page)->parent_page_number == INVALID_PAGENUM);
    TEST(records(page)[0].key == 10);
    TEST(*(int*)records(page)[0].value == 20);

    TEST_SUCCESS(bpt_test_postprocess(&bpt, &file, &buffers));
})

TEST_SUITE(bpt_insert, {

})

TEST_SUITE(shrink_root, {

})

TEST_SUITE(merge_nodes, {

})

TEST_SUITE(redistribute_nodes, {

})

TEST_SUITE(delete_entry, {

})

TEST_SUITE(bpt_delete, {

})

TEST_SUITE(destroy_tree, {
    
})

int bpt_test() {
    return swap_ubuffer_test()
        && bpt_buffering_test()
        && bpt_create_page_test()
        && bpt_free_page_test()
        && bpt_init_test()
        && bpt_release_test()
        && bpt_default_config_test()
        && bpt_test_config_test()
        && queue_test()
        && record_vec_init_test()
        && record_vec_free_test()
        && record_vec_expand_test()
        && record_vec_append_test()
        && height_test()
        && path_to_root_test()
        && cut_test()
        && find_leaf_test()
        && find_key_from_leaf_test()
        && bpt_find_test()
        && bpt_find_range_test()
        && print_leaves_test()
        && print_tree_test()
        && find_and_print_test()
        && find_and_print_range_test()
        && make_record_test()
        && make_node_test()
        && get_index_test()
        && insert_into_leaf_test()
        && insert_into_leaf_after_splitting_test()
        && insert_into_node_test()
        && insert_into_node_after_splitting_test()
        && insert_into_parent_test()
        && insert_into_new_root_test()
        && start_new_tree_test()
        && bpt_insert_test()
        && shrink_root_test()
        && merge_nodes_test()
        && redistribute_nodes_test()
        && delete_entry_test()
        && bpt_delete_test()
        && destroy_tree_test();
}
