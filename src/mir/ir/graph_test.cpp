// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#include "graph.hpp"

#include <gtest/gtest.h>

TEST(Node, eq) {
    auto n = std::make_shared<MIR::Node>(nullptr);
    auto n1 = std::make_shared<MIR::Node>(nullptr);
    auto n2 = std::make_shared<MIR::Node>(UINT32_MAX, nullptr);

    EXPECT_EQ(n, n);
    EXPECT_EQ(n1, n1);
    ASSERT_NE(n, n1);
    EXPECT_EQ(n2, n2);
}

TEST(Node, iter_pre_fix) {
    auto n = std::make_shared<MIR::Node>(nullptr);
    auto n1 = std::make_shared<MIR::Node>(nullptr);
    auto n2 = std::make_shared<MIR::Node>(nullptr);
    auto n3 = std::make_shared<MIR::Node>(nullptr);
    MIR::link_nodes(n, n1);
    MIR::link_nodes(n, n2);
    MIR::link_nodes(n1, n3);
    MIR::link_nodes(n2, n3);

    auto itr = n->begin();
    ASSERT_EQ(*itr, *n) << "Got: " << itr->id << ", but expected: " << n1->id << std::endl;
    ASSERT_NE(++itr, n->end());
    ASSERT_EQ(*itr, *n1) << "Got: " << itr->id << ", but expected: " << n1->id << std::endl;
    ASSERT_NE(++itr, n->end());
    ASSERT_EQ(*itr, *n2) << "Got: " << itr->id << ", but expected: " << n2->id << std::endl;
    ASSERT_NE(++itr, n->end());
    ASSERT_EQ(*itr, *n3) << "Got: " << itr->id << ", but expected: " << n3->id << std::endl;

    ASSERT_EQ(++itr, n->end()) << "Got: " << itr->id << ", but expected : " << n->end()->id
                               << std::endl;
}

TEST(Node, iter_post_fix) {
    auto n = std::make_shared<MIR::Node>(nullptr);
    auto n1 = std::make_shared<MIR::Node>(nullptr);
    auto n2 = std::make_shared<MIR::Node>(nullptr);
    auto n3 = std::make_shared<MIR::Node>(nullptr);
    MIR::link_nodes(n, n1);
    MIR::link_nodes(n, n2);
    MIR::link_nodes(n1, n3);
    MIR::link_nodes(n2, n3);

    auto itr = n->begin();
    ASSERT_EQ(*itr++, *n) << "Got: " << itr->id << ", but expected: " << n1->id << std::endl;
    ASSERT_NE(itr, n->end());
    ASSERT_EQ(*itr++, *n1) << "Got: " << itr->id << ", but expected: " << n1->id << std::endl;
    ASSERT_NE(itr, n->end());
    ASSERT_EQ(*itr++, *n2) << "Got: " << itr->id << ", but expected: " << n2->id << std::endl;
    ASSERT_NE(itr, n->end());
    ASSERT_EQ(*itr++, *n3) << "Got: " << itr->id << ", but expected: " << n3->id << std::endl;

    ASSERT_EQ(itr, n->end()) << "Got: " << itr->id << ", but expected : " << n->end()->id
                             << std::endl;
}

TEST(Node, iter_loop) {
    auto n = std::make_shared<MIR::Node>(0, nullptr);
    auto n1 = std::make_shared<MIR::Node>(1, nullptr);
    auto n2 = std::make_shared<MIR::Node>(2, nullptr);
    auto n3 = std::make_shared<MIR::Node>(3, nullptr);
    MIR::link_nodes(n, n1);
    MIR::link_nodes(n, n2);
    MIR::link_nodes(n1, n3);
    MIR::link_nodes(n2, n3);

    uint32_t counter = 0;
    for (auto i = n->begin(); i != n->end(); i++) {
        counter += i->id;
    }
    ASSERT_EQ(counter, 6);
}

TEST(Node, iter_range) {
    auto n = std::make_shared<MIR::Node>(0, nullptr);
    auto n1 = std::make_shared<MIR::Node>(1, nullptr);
    auto n2 = std::make_shared<MIR::Node>(2, nullptr);
    auto n3 = std::make_shared<MIR::Node>(3, nullptr);
    MIR::link_nodes(n, n1);
    MIR::link_nodes(n, n2);
    MIR::link_nodes(n1, n3);
    MIR::link_nodes(n2, n3);

    uint32_t counter = 0;
    for (auto & i : *n) {
        counter += i.id;
    }
    ASSERT_EQ(counter, 6);
}
