// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "graph.hpp"

#include <gtest/gtest.h>

using namespace MIR::IR;

TEST(CFG, iter_pre_fix) {
    CFG cfg{};
    Node * n0{cfg.head()};
    Node * n1{cfg.next()};
    Node * n2{cfg.next()};
    Node * n3{cfg.next()};
    link_nodes(n0, n1);
    link_nodes(n0, n2, true);
    link_nodes(n1, n3);
    link_nodes(n2, n3, true);
    link_nodes(n3, cfg.tail()); // This is done automatically in the ast_to_mir

    auto itr = cfg.begin();
    EXPECT_EQ(*itr, *n0) << "Got: " << itr->id << ", but expected: " << n0->id << std::endl;
    ASSERT_NE(++itr, cfg.end());
    EXPECT_EQ(*itr, *n1) << "Got: " << itr->id << ", but expected: " << n1->id << std::endl;
    ASSERT_NE(++itr, cfg.end());
    EXPECT_EQ(*itr, *n2) << "Got: " << itr->id << ", but expected: " << n2->id << std::endl;
    ASSERT_NE(++itr, cfg.end());
    EXPECT_EQ(*itr, *n3) << "Got: " << itr->id << ", but expected: " << n3->id << std::endl;

    ASSERT_EQ(++itr, cfg.end()) << "Got: " << itr->id << ", but expected : " << cfg.tail()->id
                                << std::endl;
}

TEST(CFG, iter_post_fix) {
    CFG cfg{};
    Node * n0{cfg.head()};
    Node * n1{cfg.next()};
    Node * n2{cfg.next()};
    Node * n3{cfg.next()};
    link_nodes(n0, n1);
    link_nodes(n0, n2, true);
    link_nodes(n1, n3);
    link_nodes(n2, n3, true);
    link_nodes(n3, cfg.tail()); // This is done automatically in the ast_to_mir

    auto itr = cfg.begin();
    EXPECT_EQ(*itr++, *n0) << "Got: " << itr->id << ", but expected: " << n0->id << std::endl;
    ASSERT_NE(itr, cfg.end());
    EXPECT_EQ(*itr++, *n1) << "Got: " << itr->id << ", but expected: " << n1->id << std::endl;
    ASSERT_NE(itr, cfg.end());
    EXPECT_EQ(*itr++, *n2) << "Got: " << itr->id << ", but expected: " << n2->id << std::endl;
    ASSERT_NE(itr, cfg.end());
    EXPECT_EQ(*itr++, *n3) << "Got: " << itr->id << ", but expected: " << n3->id << std::endl;

    ASSERT_EQ(itr, cfg.end()) << "Got: " << itr->id << ", but expected : " << cfg.tail()->id
                              << std::endl;
}

TEST(CFG, iter_for_loop) {
    CFG cfg{};
    Node * n0{cfg.head()};
    Node * n1{cfg.next()};
    Node * n2{cfg.next()};
    Node * n3{cfg.next()};
    link_nodes(n0, n1);
    link_nodes(n0, n2, true);
    link_nodes(n1, n3);
    link_nodes(n2, n3);
    link_nodes(n3, cfg.tail()); // This is done automatically in the ast_to_mir

    uint64_t counter = 0;
    for (auto itr = cfg.begin(); itr != cfg.end(); itr++) {
        counter += itr->id;
    }
    ASSERT_EQ(counter, 6);
}

TEST(CFG, iter_range) {
    CFG cfg{};
    Node * n0{cfg.head()};
    Node * n1{cfg.next()};
    Node * n2{cfg.next()};
    Node * n3{cfg.next()};
    link_nodes(n0, n1);
    link_nodes(n0, n2, true);
    link_nodes(n1, n3);
    link_nodes(n2, n3, true);
    link_nodes(n3, cfg.tail()); // This is done automatically in the ast_to_mir

    uint64_t counter = 0;
    for (auto i : cfg) {
        counter += i.id;
    }
    ASSERT_EQ(counter, 6);
}

TEST(CFG, iter_visits_predecessors_before_successors) {
    CFG cfg{};
    Node * n0{cfg.head()};
    Node * n1{cfg.next()};
    link_nodes(n0, n1);

    Node * n2{cfg.next()};
    link_nodes(n1, n2);

    Node * n3{cfg.next()};
    link_nodes(n0, n3, true);

    Node * n4{cfg.next()};
    link_nodes(n2, n4);
    link_nodes(n3, n4);
    link_nodes(n4, cfg.tail()); // This is done automatically in the ast_to_mir

    auto itr = cfg.begin();
    EXPECT_EQ(*itr, *n0) << "Got: " << itr->id << ", but expected: " << n0->id << std::endl;
    itr++;
    EXPECT_EQ(*itr, *n1) << "Got: " << itr->id << ", but expected: " << n1->id << std::endl;
    itr++;
    EXPECT_EQ(*itr, *n3) << "Got: " << itr->id << ", but expected: " << n3->id << std::endl;
    itr++;
    EXPECT_EQ(*itr, *n2) << "Got: " << itr->id << ", but expected: " << n2->id << std::endl;
    itr++;
    EXPECT_EQ(*itr, *n4) << "Got: " << itr->id << ", but expected: " << n4->id << std::endl;
    itr++;
    EXPECT_EQ(itr, cfg.end()) << "Got: " << itr->id << ", but expected : " << cfg.tail()->id
                              << std::endl;
}

TEST(CFG, iter_visits_predecessors_before_successors_2) {
    CFG cfg{};
    Node * n0{cfg.head()};
    Node * n1{cfg.next()};
    link_nodes(n0, n1, true);

    Node * n2{cfg.next()};
    link_nodes(n1, n2);

    Node * n3{cfg.next()};
    link_nodes(n0, n3);

    Node * n4{cfg.next()};
    link_nodes(n2, n4);
    link_nodes(n3, n4);
    link_nodes(n4, cfg.tail()); // This is done automatically in the ast_to_mir

    auto itr = cfg.begin();
    EXPECT_EQ(*itr, *n0) << "Got: " << itr->id << ", but expected: " << n0->id << std::endl;
    itr++;
    EXPECT_EQ(*itr, *n3) << "Got: " << itr->id << ", but expected: " << n3->id << std::endl;
    itr++;
    EXPECT_EQ(*itr, *n1) << "Got: " << itr->id << ", but expected: " << n1->id << std::endl;
    itr++;
    EXPECT_EQ(*itr, *n2) << "Got: " << itr->id << ", but expected: " << n2->id << std::endl;
    itr++;
    EXPECT_EQ(*itr, *n4) << "Got: " << itr->id << ", but expected: " << n4->id << std::endl;
    itr++;
    EXPECT_EQ(itr, cfg.end()) << "Got: " << itr->id << ", but expected : " << cfg.tail()->id
                              << std::endl;
}

TEST(CFG, iter_visits_predecessors_before_successors_loop) {
    CFG cfg{};
    Node * preamble{cfg.head()};

    Node * header{cfg.next()};
    header->loop_header = true;
    link_nodes(preamble, header);

    Node * tail{cfg.next()};
    link_nodes(header, tail, true);
    link_nodes(tail, cfg.tail()); // This is done automatically in the ast_to_mir

    Node * body{cfg.next()};
    link_nodes(header, body);
    link_nodes(body, header);

    auto itr = cfg.begin();
    EXPECT_EQ(*itr, *preamble) << "Got: " << itr->id << ", but expected: " << preamble->id
                               << std::endl;
    itr++;
    EXPECT_EQ(*itr, *header) << "Got: " << itr->id << ", but expected: " << header->id << std::endl;
    itr++;
    EXPECT_EQ(*itr, *body) << "Got: " << itr->id << ", but expected: " << body->id << std::endl;
    itr++;
    EXPECT_EQ(*itr, *tail) << "Got: " << itr->id << ", but expected: " << tail->id << std::endl;
    itr++;
    EXPECT_EQ(itr, cfg.end()) << "Got: " << itr->id << ", but expected : " << cfg.tail()->id
                              << std::endl;
}

TEST(CFG, iter_visits_predecessors_before_successors_loop_2) {
    CFG cfg{};
    Node * preamble{cfg.head()};

    Node * header{cfg.next()};
    header->loop_header = true;
    link_nodes(preamble, header);

    Node * tail{cfg.next()};
    link_nodes(header, tail, true);
    link_nodes(tail, cfg.tail()); // This is done automatically in the ast_to_mir

    Node * body{cfg.next()};
    link_nodes(header, body);

    Node * body2{cfg.next()};
    link_nodes(body, body2);
    link_nodes(body2, header);

    auto itr = cfg.begin();
    EXPECT_EQ(*itr, *preamble) << "Got: " << itr->id << ", but expected: " << preamble->id
                               << std::endl;
    itr++;
    EXPECT_EQ(*itr, *header) << "Got: " << itr->id << ", but expected: " << header->id << std::endl;
    itr++;
    EXPECT_EQ(*itr, *body) << "Got: " << itr->id << ", but expected: " << body->id << std::endl;
    itr++;
    EXPECT_EQ(*itr, *tail) << "Got: " << itr->id << ", but expected: " << tail->id << std::endl;
    itr++;
    EXPECT_EQ(*itr, *body2) << "Got: " << itr->id << ", but expected: " << body2->id << std::endl;
    itr++;
    EXPECT_EQ(itr, cfg.end()) << "Got: " << itr->id << ", but expected : " << cfg.tail()->id
                              << std::endl;
}

TEST(Node, eq) {
    Node n{0, nullptr};
    Node n1{1, nullptr};
    Node n2{UINT32_MAX, nullptr};

    EXPECT_EQ(n, n);
    EXPECT_EQ(n1, n1);
    EXPECT_NE(n, n1);
    EXPECT_EQ(n2, n2);
}

TEST(link_nodes, tail) {
    CFG cfg{};
    Node * n0{cfg.head()};
    Node * n1{cfg.next()};
    Node * n2{cfg.next()};

    link_nodes(n0, n1);
    link_nodes(n1, cfg.tail());
    ASSERT_EQ(n0->left_successor(), n1);
    ASSERT_EQ(n1->left_successor(), cfg.tail());

    link_nodes(n1, n2);

    EXPECT_EQ(n0->left_successor(), n1);
    EXPECT_EQ(n1->left_successor(), n2);
    EXPECT_EQ(n2->left_successor(), cfg.tail());
}

TEST(reparent, simple) {
    Node n{0, nullptr};
    Node n1{1, nullptr};
    Node n2{2, nullptr};
    link_nodes(&n, &n1);
    link_nodes(&n1, &n2);

    Node n3{3, nullptr};
    reparent(&n, &n3);
    EXPECT_FALSE(n.left_successor());
    EXPECT_EQ(n3.left_successor(), &n1);
    EXPECT_TRUE(n1.predecessors.find(&n) == n1.predecessors.end());
    EXPECT_TRUE(n1.predecessors.find(&n3) != n1.predecessors.end());
}

TEST(Node, depth_simple) {
    CFG cfg{};
    Node * n0{cfg.head()};
    Node * n1{cfg.next()};
    Node * n2{cfg.next()};

    link_nodes(n0, n1);
    link_nodes(n1, n2);
    link_nodes(n2, cfg.tail());

    Node * n3{cfg.next()};
    link_nodes(n3, cfg.tail());
    link_nodes(n2, n3);

    EXPECT_EQ(n0->depth, 0);
    EXPECT_EQ(n1->depth, 1);
    EXPECT_EQ(n2->depth, 2);
    EXPECT_EQ(n3->depth, 3);
    EXPECT_EQ(cfg.tail()->depth, 4);
}

TEST(Node, no_change) {
    CFG cfg{};
    Node * n0{cfg.head()};
    Node * n1{cfg.next()};
    Node * n2{cfg.next()};

    link_nodes(n0, n1);
    link_nodes(n1, n2);
    link_nodes(n2, cfg.tail());

    Node * n3{cfg.next()};
    Node * n4{cfg.next()};
    Node * n5{cfg.next()};
    Node * n6{cfg.next()};
    link_nodes(n1, n3, true);
    link_nodes(n3, n4);
    link_nodes(n4, n5);
    link_nodes(n5, n6);
    link_nodes(n6, cfg.tail());

    EXPECT_EQ(n0->depth, 0);
    EXPECT_EQ(n1->depth, 1);
    EXPECT_EQ(n2->depth, 2);
    EXPECT_EQ(n3->depth, 2);
    EXPECT_EQ(n4->depth, 3);
    EXPECT_EQ(n5->depth, 4);
    EXPECT_EQ(n6->depth, 5);
    EXPECT_EQ(cfg.tail()->depth, 6);

    link_nodes(n2, n6);
    EXPECT_EQ(n6->depth, 5);
    EXPECT_EQ(cfg.tail()->depth, 6);
}

TEST(Node, deep) {
    CFG cfg{};
    Node * n0{cfg.head()};
    Node * n1{cfg.next()};
    Node * n2{cfg.next()};

    link_nodes(n0, n1);
    link_nodes(n1, n2);
    link_nodes(n2, cfg.tail());

    Node * n3{cfg.next()};
    Node * n4{cfg.next()};
    Node * n5{cfg.next()};
    Node * n6{cfg.next()};
    link_nodes(n1, n3, true);
    link_nodes(n3, n4);
    link_nodes(n4, n5);
    link_nodes(n5, n6);
    link_nodes(n6, cfg.tail());

    EXPECT_EQ(n0->depth, 0);
    EXPECT_EQ(n1->depth, 1);
    EXPECT_EQ(n2->depth, 2);
    EXPECT_EQ(n3->depth, 2);
    EXPECT_EQ(n4->depth, 3);
    EXPECT_EQ(n5->depth, 4);
    EXPECT_EQ(n6->depth, 5);
    EXPECT_EQ(cfg.tail()->depth, 6);

    link_nodes(n6, n2);
    // Should not have changed
    EXPECT_EQ(n0->depth, 0);
    EXPECT_EQ(n3->depth, 2);
    EXPECT_EQ(n4->depth, 3);
    EXPECT_EQ(n5->depth, 4);
    EXPECT_EQ(n6->depth, 5);

    // Should have changed
    EXPECT_EQ(n2->depth, 6);
    EXPECT_EQ(cfg.tail()->depth, 7);
}
