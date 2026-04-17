// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "graph.hpp"

#include <gtest/gtest.h>

using namespace MIR::IR;

TEST(CFG, iter_pre_fix) {
    auto n0 = std::make_shared<Node>(0, nullptr);
    auto n1 = std::make_shared<Node>(1, nullptr);
    auto n2 = std::make_shared<Node>(2, nullptr);
    auto n3 = std::make_shared<Node>(3, nullptr);
    link_nodes(n0, n1);
    link_nodes(n0, n2, true);
    link_nodes(n1, n3);
    link_nodes(n2, n3, true);

    CFG cfg{n0};

    auto itr = cfg.begin();
    EXPECT_EQ(*itr, n0) << "Got: " << (*itr)->id << ", but expected: " << n0->id << std::endl;
    ASSERT_NE(++itr, cfg.end());
    EXPECT_EQ(*itr, n1) << "Got: " << (*itr)->id << ", but expected: " << n1->id << std::endl;
    ASSERT_NE(++itr, cfg.end());
    EXPECT_EQ(*itr, n2) << "Got: " << (*itr)->id << ", but expected: " << n2->id << std::endl;
    ASSERT_NE(++itr, cfg.end());
    EXPECT_EQ(*itr, n3) << "Got: " << (*itr)->id << ", but expected: " << n3->id << std::endl;

    ASSERT_EQ(++itr, cfg.end()) << "Got: " << (*itr)->id << ", but expected : " << (*cfg.end())->id
                                << std::endl;
}

TEST(CFG, iter_post_fix) {
    auto n0 = std::make_shared<Node>(nullptr);
    auto n1 = std::make_shared<Node>(nullptr);
    auto n2 = std::make_shared<Node>(nullptr);
    auto n3 = std::make_shared<Node>(nullptr);
    link_nodes(n0, n1);
    link_nodes(n0, n2, true);
    link_nodes(n1, n3);
    link_nodes(n2, n3, true);

    CFG cfg{n0};

    auto itr = cfg.begin();
    EXPECT_EQ(*itr++, n0) << "Got: " << (*itr)->id << ", but expected: " << n0->id << std::endl;
    ASSERT_NE(itr, cfg.end());
    EXPECT_EQ(*itr++, n1) << "Got: " << (*itr)->id << ", but expected: " << n1->id << std::endl;
    ASSERT_NE(itr, cfg.end());
    EXPECT_EQ(*itr++, n2) << "Got: " << (*itr)->id << ", but expected: " << n2->id << std::endl;
    ASSERT_NE(itr, cfg.end());
    EXPECT_EQ(*itr++, n3) << "Got: " << (*itr)->id << ", but expected: " << n3->id << std::endl;

    ASSERT_EQ(itr, cfg.end()) << "Got: " << (*itr)->id << ", but expected : " << (*cfg.end())->id
                              << std::endl;
}

TEST(CFG, iter_for_loop) {
    auto n0 = std::make_shared<Node>(0, nullptr);
    auto n1 = std::make_shared<Node>(1, nullptr);
    auto n2 = std::make_shared<Node>(2, nullptr);
    auto n3 = std::make_shared<Node>(3, nullptr);
    link_nodes(n0, n1);
    link_nodes(n0, n2, true);
    link_nodes(n1, n3);
    link_nodes(n2, n3);

    CFG cfg{n0};

    uint32_t counter = 0;
    for (auto itr = cfg.begin(); itr != cfg.end(); itr++) {
        counter += (*itr)->id;
    }
    ASSERT_EQ(counter, 6);
}

TEST(CFG, iter_range) {
    auto n0 = std::make_shared<Node>(0, nullptr);
    auto n1 = std::make_shared<Node>(1, nullptr);
    auto n2 = std::make_shared<Node>(2, nullptr);
    auto n3 = std::make_shared<Node>(3, nullptr);
    link_nodes(n0, n1);
    link_nodes(n0, n2, true);
    link_nodes(n1, n3);
    link_nodes(n2, n3, true);

    CFG cfg{n0};

    uint32_t counter = 0;
    for (auto i : cfg) {
        counter += i->id;
    }
    ASSERT_EQ(counter, 6);
}

TEST(CFG, iter_visits_predecessors_before_successors) {
    auto n0 = std::make_shared<Node>(0, nullptr);
    auto n1 = std::make_shared<Node>(1, nullptr);
    link_nodes(n0, n1);

    auto n2 = std::make_shared<Node>(2, nullptr);
    link_nodes(n1, n2);

    auto n3 = std::make_shared<Node>(3, nullptr);
    link_nodes(n0, n3, true);

    auto n4 = std::make_shared<Node>(4, nullptr);
    link_nodes(n2, n4);
    link_nodes(n3, n4);

    CFG cfg{n0};

    auto itr = cfg.begin();
    EXPECT_EQ(*itr, n0) << "Got: " << (*itr)->id << ", but expected: " << n0->id << std::endl;
    itr++;
    EXPECT_EQ(*itr, n1) << "Got: " << (*itr)->id << ", but expected: " << n1->id << std::endl;
    itr++;
    EXPECT_EQ(*itr, n3) << "Got: " << (*itr)->id << ", but expected: " << n3->id << std::endl;
    itr++;
    EXPECT_EQ(*itr, n2) << "Got: " << (*itr)->id << ", but expected: " << n2->id << std::endl;
    itr++;
    EXPECT_EQ(*itr, n4) << "Got: " << (*itr)->id << ", but expected: " << n4->id << std::endl;
    itr++;
    EXPECT_EQ(itr, cfg.end()) << "Got: " << (*itr)->id << ", but expected : " << (*cfg.end())->id
                              << std::endl;
}

TEST(CFG, iter_visits_predecessors_before_successors_2) {
    auto n0 = std::make_shared<Node>(0, nullptr);
    auto n1 = std::make_shared<Node>(1, nullptr);
    link_nodes(n0, n1, true);

    auto n2 = std::make_shared<Node>(2, nullptr);
    link_nodes(n1, n2);

    auto n3 = std::make_shared<Node>(3, nullptr);
    link_nodes(n0, n3);

    auto n4 = std::make_shared<Node>(4, nullptr);
    link_nodes(n2, n4);
    link_nodes(n3, n4);

    CFG cfg{n0};

    auto itr = cfg.begin();
    EXPECT_EQ(*itr, n0) << "Got: " << (*itr)->id << ", but expected: " << n0->id << std::endl;
    itr++;
    EXPECT_EQ(*itr, n3) << "Got: " << (*itr)->id << ", but expected: " << n3->id << std::endl;
    itr++;
    EXPECT_EQ(*itr, n1) << "Got: " << (*itr)->id << ", but expected: " << n1->id << std::endl;
    itr++;
    EXPECT_EQ(*itr, n2) << "Got: " << (*itr)->id << ", but expected: " << n2->id << std::endl;
    itr++;
    EXPECT_EQ(*itr, n4) << "Got: " << (*itr)->id << ", but expected: " << n4->id << std::endl;
    itr++;
    EXPECT_EQ(itr, cfg.end()) << "Got: " << (*itr)->id << ", but expected : " << (*cfg.end())->id
                              << std::endl;
}

TEST(CFG, iter_visits_predecessors_before_successors_loop) {
    auto preamble = std::make_shared<Node>(0, nullptr);

    auto header = std::make_shared<Node>(1, nullptr);
    header->loop_header = true;
    link_nodes(preamble, header);

    auto tail = std::make_shared<Node>(2, nullptr);
    link_nodes(header, tail, true);

    auto body = std::make_shared<Node>(3, nullptr);
    link_nodes(header, body);
    link_nodes(body, header);

    CFG cfg{preamble};

    auto itr = cfg.begin();
    EXPECT_EQ(*itr, preamble) << "Got: " << (*itr)->id << ", but expected: " << preamble->id
                              << std::endl;
    itr++;
    EXPECT_EQ(*itr, header) << "Got: " << (*itr)->id << ", but expected: " << header->id
                            << std::endl;
    itr++;
    EXPECT_EQ(*itr, body) << "Got: " << (*itr)->id << ", but expected: " << body->id << std::endl;
    itr++;
    EXPECT_EQ(*itr, tail) << "Got: " << (*itr)->id << ", but expected: " << tail->id << std::endl;
    itr++;
    EXPECT_EQ(itr, cfg.end()) << "Got: " << (*itr)->id << ", but expected : " << (*cfg.end())->id
                              << std::endl;
}

TEST(CFG, iter_visits_predecessors_before_successors_loop_2) {
    auto preamble = std::make_shared<Node>(0, nullptr);

    auto header = std::make_shared<Node>(1, nullptr);
    header->loop_header = true;
    link_nodes(preamble, header);

    auto tail = std::make_shared<Node>(2, nullptr);
    link_nodes(header, tail, true);

    auto body = std::make_shared<Node>(3, nullptr);
    link_nodes(header, body);

    auto body2 = std::make_shared<Node>(4, nullptr);
    link_nodes(body, body2);
    link_nodes(body2, header);

    CFG cfg{preamble};

    auto itr = cfg.begin();
    EXPECT_EQ(*itr, preamble) << "Got: " << (*itr)->id << ", but expected: " << preamble->id
                              << std::endl;
    itr++;
    EXPECT_EQ(*itr, header) << "Got: " << (*itr)->id << ", but expected: " << header->id
                            << std::endl;
    itr++;
    EXPECT_EQ(*itr, body) << "Got: " << (*itr)->id << ", but expected: " << body->id << std::endl;
    itr++;
    EXPECT_EQ(*itr, tail) << "Got: " << (*itr)->id << ", but expected: " << tail->id << std::endl;
    itr++;
    EXPECT_EQ(*itr, body2) << "Got: " << (*itr)->id << ", but expected: " << body2->id << std::endl;
    itr++;
    EXPECT_EQ(itr, cfg.end()) << "Got: " << (*itr)->id << ", but expected : " << (*cfg.end())->id
                              << std::endl;
}

TEST(Node, eq) {
    auto n = std::make_shared<Node>(nullptr);
    auto n1 = std::make_shared<Node>(nullptr);
    auto n2 = std::make_shared<Node>(UINT32_MAX, nullptr);

    EXPECT_EQ(n, n);
    EXPECT_EQ(n1, n1);
    EXPECT_NE(n, n1);
    EXPECT_EQ(n2, n2);
}

TEST(Node, reparent) {
    auto n = std::make_shared<Node>(nullptr);
    auto n1 = std::make_shared<Node>(nullptr);
    auto n2 = std::make_shared<Node>(nullptr);
    link_nodes(n, n1);
    link_nodes(n1, n2);

    auto n3 = std::make_shared<Node>(nullptr);
    reparent(n, n3);
    EXPECT_FALSE(n->left_successor());
    EXPECT_EQ(n3->left_successor(), n1);
    EXPECT_TRUE(n1->predecessors.find(n) == n1->predecessors.end());
    EXPECT_TRUE(n1->predecessors.find(n3) != n1->predecessors.end());
}
