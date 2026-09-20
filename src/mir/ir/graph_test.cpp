// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "graph.hpp"

#include <gtest/gtest.h>

using namespace MIR::IR;

TEST(CFG, sort) {
    CFG cfg;
    BasicBlock * n0{cfg.head()};
    BasicBlock * n3{cfg.next()};
    BasicBlock * n1{cfg.next()};
    BasicBlock * n2{cfg.next()};

    link_blocks(n0, n1);
    link_blocks(n0, n2, true);
    link_blocks(n1, n3);
    link_blocks(n2, n3);
    cfg.sort();

    EXPECT_EQ(*cfg.nodes.at(0), *n0)
        << "expected: " << n0->id << ", but got: " << cfg.nodes.at(0)->id;
    EXPECT_EQ(*cfg.nodes.at(1), *n2)
        << "expected: " << n2->id << ", but got: " << cfg.nodes.at(1)->id;
    EXPECT_EQ(*cfg.nodes.at(2), *n1)
        << "expected: " << n2->id << ", but got: " << cfg.nodes.at(2)->id;
    EXPECT_EQ(*cfg.nodes.at(3), *n3)
        << "expected: " << n3->id << ", but got: " << cfg.nodes.at(3)->id;
}

class NodeIterTest : public testing::Test {
  protected:
    NodeIterTest() : cfg{}, n0{cfg.head()}, n3{cfg.next()}, n1{cfg.next()}, n2{cfg.next()} {
        /*
         *           O n0
         *          / \
         *      n1 O   O n2
         *          \ /
         *           O n3
         */
        link_blocks(n0, n1);
        link_blocks(n0, n2, true);
        link_blocks(n1, n3);
        link_blocks(n2, n3);
        cfg.sort();
    }

    CFG cfg;
    BasicBlock * n0;
    BasicBlock * n3;
    BasicBlock * n1;
    BasicBlock * n2;
};

TEST_F(NodeIterTest, prefix_forward) {
    auto itr = cfg.begin();
    EXPECT_EQ(**itr, *n0) << "Got: " << (*itr)->id << ", but expected: " << n0->id << std::endl;
    ASSERT_NE(++itr, cfg.end());
    EXPECT_EQ(**itr, *n2) << "Got: " << (*itr)->id << ", but expected: " << n2->id << std::endl;
    ASSERT_NE(++itr, cfg.end());
    EXPECT_EQ(**itr, *n1) << "Got: " << (*itr)->id << ", but expected: " << n1->id << std::endl;
    ASSERT_NE(++itr, cfg.end());
    EXPECT_EQ(**itr, *n3) << "Got: " << (*itr)->id << ", but expected: " << n3->id << std::endl;

    ASSERT_EQ(++itr, cfg.end()) << "Got: " << (*itr)->id << ", but expected : tail sentinel"
                                << std::endl;
}

TEST_F(NodeIterTest, node_prefix_forward) {
    auto itr = n0->begin();
    EXPECT_EQ(**itr, *n2) << "Got: " << (*itr)->id << ", but expected: " << n2->id << std::endl;
    ASSERT_NE(++itr, n0->end());
    EXPECT_EQ(**itr, *n1) << "Got: " << (*itr)->id << ", but expected: " << n1->id << std::endl;
    ASSERT_NE(++itr, n0->end());
    EXPECT_EQ(**itr, *n3) << "Got: " << (*itr)->id << ", but expected: " << n3->id << std::endl;

    ASSERT_EQ(++itr, n0->end()) << "Got: " << (*itr)->id << ", but expected : tail sentinel"
                                << std::endl;
}

TEST_F(NodeIterTest, node_prefix_forward_partial) {
    auto itr = n2->begin();
    EXPECT_EQ(**itr, *n3) << "Got: " << (*itr)->id << ", but expected: " << n3->id << std::endl;

    ASSERT_EQ(++itr, n0->end()) << "Got: " << (*itr)->id << ", but expected : tail sentinel"
                                << std::endl;
}

TEST_F(NodeIterTest, prefix_reverse) {
    auto itr = cfg.rbegin();
    EXPECT_EQ(**itr, *n3) << "Got: " << (*itr)->id << ", but expected: " << n3->id << std::endl;
    ASSERT_NE(++itr, cfg.rend());
    EXPECT_EQ(**itr, *n1) << "Got: " << (*itr)->id << ", but expected: " << n1->id << std::endl;
    ASSERT_NE(++itr, cfg.rend());
    EXPECT_EQ(**itr, *n2) << "Got: " << (*itr)->id << ", but expected: " << n2->id << std::endl;
    ASSERT_NE(++itr, cfg.rend());
    EXPECT_EQ(**itr, *n0) << "Got: " << (*itr)->id << ", but expected: " << n0->id << std::endl;

    ASSERT_EQ(++itr, cfg.rend()) << "Got: " << (*itr)->id << ", but expected : tail sentinel"
                                 << std::endl;
}

TEST_F(NodeIterTest, node_prefix_reverse) {
    auto itr = n3->rbegin();
    EXPECT_EQ(**itr, *n1) << "Got: " << (*itr)->id << ", but expected: " << n1->id << std::endl;
    ASSERT_NE(++itr, n3->rend());
    EXPECT_EQ(**itr, *n2) << "Got: " << (*itr)->id << ", but expected: " << n2->id << std::endl;
    ASSERT_NE(++itr, n3->rend());
    EXPECT_EQ(**itr, *n0) << "Got: " << (*itr)->id << ", but expected: " << n0->id << std::endl;

    ASSERT_EQ(++itr, n3->rend()) << "Got: " << (*itr)->id << ", but expected : tail sentinel"
                                 << std::endl;
}

TEST_F(NodeIterTest, node_prefix_reverse_partial) {
    auto itr = n2->rbegin();
    EXPECT_EQ(**itr, *n0) << "Got: " << (*itr)->id << ", but expected: " << n0->id << std::endl;

    ASSERT_EQ(++itr, n3->rend()) << "Got: " << (*itr)->id << ", but expected : tail sentinel"
                                 << std::endl;
}

TEST_F(NodeIterTest, postfix_forward) {
    auto itr = cfg.begin();
    EXPECT_EQ(**(itr++), *n0) << "Got: " << (*itr)->id << ", but expected: " << n0->id << std::endl;
    ASSERT_NE(itr, cfg.end());
    EXPECT_EQ(**(itr++), *n2) << "Got: " << (*itr)->id << ", but expected: " << n2->id << std::endl;
    ASSERT_NE(itr, cfg.end());
    EXPECT_EQ(**(itr++), *n1) << "Got: " << (*itr)->id << ", but expected: " << n1->id << std::endl;
    ASSERT_NE(itr, cfg.end());
    EXPECT_EQ(**(itr++), *n3) << "Got: " << (*itr)->id << ", but expected: " << n3->id << std::endl;

    ASSERT_EQ(itr, cfg.end()) << "Got: " << (*itr)->id << ", but expected : tail sentinel"
                              << std::endl;
}

TEST_F(NodeIterTest, postfix_reverse) {
    auto itr = cfg.rbegin();
    EXPECT_EQ(**(itr++), *n3) << "Got: " << (*itr)->id << ", but expected: " << n0->id << std::endl;
    ASSERT_NE(itr, cfg.rend());
    EXPECT_EQ(**(itr++), *n1) << "Got: " << (*itr)->id << ", but expected: " << n2->id << std::endl;
    ASSERT_NE(itr, cfg.rend());
    EXPECT_EQ(**(itr++), *n2) << "Got: " << (*itr)->id << ", but expected: " << n1->id << std::endl;
    ASSERT_NE(itr, cfg.rend());
    EXPECT_EQ(**(itr++), *n0) << "Got: " << (*itr)->id << ", but expected: " << n3->id << std::endl;

    ASSERT_EQ(itr, cfg.rend()) << "Got: " << (*itr)->id << ", but expected : tail sentinel"
                               << std::endl;
}

TEST_F(NodeIterTest, for_loop_forward) {
    uint64_t counter = 0;
    for (auto itr = cfg.begin(); itr != cfg.end(); itr++) {
        counter += (*itr)->id;
    }
    ASSERT_EQ(counter, 6);
}

TEST_F(NodeIterTest, for_loop_reverse) {
    uint64_t counter = 0;
    for (auto itr = cfg.rbegin(); itr != cfg.rend(); itr++) {
        counter += (*itr)->id;
    }
    ASSERT_EQ(counter, 6);
}

TEST_F(NodeIterTest, iter_range_forward) {
    uint64_t counter = 0;
    for (auto & i : cfg) {
        counter += i->id;
    }
    ASSERT_EQ(counter, 6);
}

class NodeDiamondIteratorTest : public testing::Test {
  protected:
    NodeDiamondIteratorTest()
        : cfg{}, n0{cfg.head()}, n1{cfg.next()}, n2{cfg.next()}, n3{cfg.next()}, n4{cfg.next()} {
        /*
         *           O n0
         *          / \
         *      n1 O   O n3
         *         |   |
         *      n2 O   |
         *          \ /
         *           O n4
         */
        link_blocks(n0, n1);
        link_blocks(n1, n2);
        link_blocks(n0, n3, true);
        link_blocks(n2, n4);
        link_blocks(n3, n4);
    }

    CFG cfg;
    BasicBlock * n0;
    BasicBlock * n1;
    BasicBlock * n2;
    BasicBlock * n3;
    BasicBlock * n4;
};

TEST_F(NodeDiamondIteratorTest, visit_predecessors_before_successors_forward) {
    auto itr = cfg.begin();
    EXPECT_EQ(**itr, *n0) << "Got: " << (*itr)->id << ", but expected: " << n0->id << std::endl;
    itr++;
    EXPECT_EQ(**itr, *n3) << "Got: " << (*itr)->id << ", but expected: " << n3->id << std::endl;
    itr++;
    EXPECT_EQ(**itr, *n1) << "Got: " << (*itr)->id << ", but expected: " << n1->id << std::endl;
    itr++;
    EXPECT_EQ(**itr, *n2) << "Got: " << (*itr)->id << ", but expected: " << n2->id << std::endl;
    itr++;
    EXPECT_EQ(**itr, *n4) << "Got: " << (*itr)->id << ", but expected: " << n4->id << std::endl;
    itr++;
    EXPECT_EQ(itr, cfg.end()) << "Got: " << (*itr)->id << ", but expected : tail sentinel"
                              << std::endl;
}

TEST_F(NodeDiamondIteratorTest, visit_predecessors_before_successors_reverse) {
    auto itr = cfg.rbegin();
    EXPECT_EQ(**itr, *n4) << "Got: " << (*itr)->id << ", but expected: " << n0->id << std::endl;
    itr++;
    EXPECT_EQ(**itr, *n2) << "Got: " << (*itr)->id << ", but expected: " << n1->id << std::endl;
    itr++;
    EXPECT_EQ(**itr, *n1) << "Got: " << (*itr)->id << ", but expected: " << n2->id << std::endl;
    itr++;
    EXPECT_EQ(**itr, *n3) << "Got: " << (*itr)->id << ", but expected: " << n3->id << std::endl;
    itr++;
    EXPECT_EQ(**itr, *n0) << "Got: " << (*itr)->id << ", but expected: " << n4->id << std::endl;
    itr++;
    EXPECT_EQ(itr, cfg.rend()) << "Got: " << (*itr)->id << ", but expected : tail sentinel"
                               << std::endl;
}

class NodeDiamondIteratorTest2 : public testing::Test {
  protected:
    NodeDiamondIteratorTest2()
        : cfg{}, n0{cfg.head()}, n1{cfg.next()}, n2{cfg.next()}, n3{cfg.next()}, n4{cfg.next()} {
        /*
         *           O n0
         *          / \
         *      n3 O   O n1
         *         |   |
         *         \   O n2
         *          \ /
         *           O n4
         */
        link_blocks(n0, n1, true);
        link_blocks(n1, n2);
        link_blocks(n0, n3);
        link_blocks(n2, n4);
        link_blocks(n3, n4);
    }

    CFG cfg;
    BasicBlock * n0;
    BasicBlock * n1;
    BasicBlock * n2;
    BasicBlock * n3;
    BasicBlock * n4;
};

TEST_F(NodeDiamondIteratorTest2, visit_predecessors_before_successors_forward) {
    auto itr = cfg.cbegin();
    EXPECT_EQ(**itr, *n0) << "Got: " << (*itr)->id << ", but expected: " << n0->id << std::endl;
    itr++;
    EXPECT_EQ(**itr, *n3) << "Got: " << (*itr)->id << ", but expected: " << n3->id << std::endl;
    itr++;
    EXPECT_EQ(**itr, *n1) << "Got: " << (*itr)->id << ", but expected: " << n1->id << std::endl;
    itr++;
    EXPECT_EQ(**itr, *n2) << "Got: " << (*itr)->id << ", but expected: " << n2->id << std::endl;
    itr++;
    EXPECT_EQ(**itr, *n4) << "Got: " << (*itr)->id << ", but expected: " << n4->id << std::endl;
    itr++;
    EXPECT_EQ(itr, cfg.cend()) << "Got: " << (*itr)->id << ", but expected : tail sentinel"
                               << std::endl;
}

TEST_F(NodeDiamondIteratorTest2, visit_predecessors_before_successors_reverse) {
    auto itr = cfg.crbegin();
    EXPECT_EQ(**itr, *n4) << "Got: " << (*itr)->id << ", but expected: " << n0->id << std::endl;
    itr++;
    EXPECT_EQ(**itr, *n2) << "Got: " << (*itr)->id << ", but expected: " << n3->id << std::endl;
    itr++;
    EXPECT_EQ(**itr, *n1) << "Got: " << (*itr)->id << ", but expected: " << n2->id << std::endl;
    itr++;
    EXPECT_EQ(**itr, *n3) << "Got: " << (*itr)->id << ", but expected: " << n1->id << std::endl;
    itr++;
    EXPECT_EQ(**itr, *n0) << "Got: " << (*itr)->id << ", but expected: " << n4->id << std::endl;
    itr++;
    EXPECT_EQ(itr, cfg.crend()) << "Got: " << (*itr)->id << ", but expected : tail sentinel"
                                << std::endl;
}

class NodeBasicLoopTest : public testing::Test {
  protected:
    NodeBasicLoopTest()
        : cfg{}, preamble{cfg.head()}, header{cfg.next()}, tail{cfg.next()}, body{cfg.next()} {
        /*
         *           O preamble
         *           |
         *           O header
         *          / \
         *    body O   |
         *            /
         *           O tail
         */
        header->loop_header = true;
        link_blocks(preamble, header);
        link_blocks(header, tail, true);
        link_blocks(header, body);
        link_blocks(body, header);
    };

    CFG cfg{};
    BasicBlock * preamble;
    BasicBlock * header;
    BasicBlock * tail;
    BasicBlock * body;
};

TEST_F(NodeBasicLoopTest, iter_visits_predecessors_before_successors_forward) {
    auto itr = cfg.begin();
    EXPECT_EQ(**itr, *preamble) << "Got: " << (*itr)->id << ", but expected: " << preamble->id
                                << std::endl;
    itr++;
    EXPECT_EQ(**itr, *header) << "Got: " << (*itr)->id << ", but expected: " << header->id
                              << std::endl;
    itr++;
    EXPECT_EQ(**itr, *body) << "Got: " << (*itr)->id << ", but expected: " << body->id << std::endl;
    itr++;
    EXPECT_EQ(**itr, *tail) << "Got: " << (*itr)->id << ", but expected: " << tail->id << std::endl;
    itr++;
    EXPECT_EQ(itr, cfg.end()) << "Got: " << (*itr)->id << ", but expected : tail sentinel"
                              << std::endl;
}

TEST_F(NodeBasicLoopTest, depth) {
    /*
     *           O preamble
     *           |
     *           O header
     *          / \
     *    body O   O tail
     */
    EXPECT_EQ(preamble->depth, 0);
    EXPECT_EQ(header->depth, 1);
    EXPECT_EQ(body->depth, 2);
    EXPECT_EQ(tail->depth, 2);
}

class NodeLoopMultiBodyTest : public testing::Test {
  protected:
    NodeLoopMultiBodyTest()
        : cfg{}, preamble{cfg.head()}, header{cfg.next()}, tail{cfg.next()}, body{cfg.next()},
          body2{cfg.next()} {
        /*
         *           O preamble
         *           |
         *           O header
         *          / \
         *    body O   |
         *            /
         *           O tail
         */
        header->loop_header = true;
        link_blocks(preamble, header);
        link_blocks(header, tail, true);
        link_blocks(header, body);
        link_blocks(body, body2);
        link_blocks(body2, header);
    };

    CFG cfg{};
    BasicBlock * preamble;
    BasicBlock * header;
    BasicBlock * tail;
    BasicBlock * body;
    BasicBlock * body2;
};

TEST_F(NodeLoopMultiBodyTest, depth) {
    /*
     *           O preamble
     *           |
     *           O header
     *          / \
     *    body O   O tail
     */
    EXPECT_EQ(preamble->depth, 0);
    EXPECT_EQ(header->depth, 1);
    EXPECT_EQ(body->depth, 2);
    EXPECT_EQ(body2->depth, 3);
    EXPECT_EQ(tail->depth, 2);
}

TEST_F(NodeLoopMultiBodyTest, iter_visits_predecessors_before_successors) {
    /*
     *           O preamble
     *           |
     *         > O header
     *        / / \
     *       | O   |  body
     *        \|   |
     *         O   |  body2
     *            /
     *           O tail
     */
    auto itr = cfg.cbegin();
    EXPECT_EQ(**itr, *preamble) << "Got: " << (*itr)->id << ", but expected: " << preamble->id
                                << std::endl;
    itr++;
    EXPECT_EQ(**itr, *header) << "Got: " << (*itr)->id << ", but expected: " << header->id
                              << std::endl;
    itr++;
    EXPECT_EQ(**itr, *body) << "Got: " << (*itr)->id << ", but expected: " << body->id << std::endl;
    itr++;
    EXPECT_EQ(**itr, *tail) << "Got: " << (*itr)->id << ", but expected: " << tail->id << std::endl;
    itr++;
    EXPECT_EQ(**itr, *body2) << "Got: " << (*itr)->id << ", but expected: " << body2->id
                             << std::endl;
    itr++;
    EXPECT_EQ(itr, cfg.cend()) << "Got: " << (*itr)->id << ", but expected : tail sentinel"
                               << std::endl;
}

TEST(Node, eq) {
    CFG cfg{};
    BasicBlock * n0{cfg.head()};
    BasicBlock * n1{cfg.next()};
    BasicBlock * n2{cfg.next()};

    EXPECT_EQ(*n0, *n0);
    EXPECT_EQ(*n1, *n1);
    EXPECT_NE(*n0, *n1);
    EXPECT_EQ(*n2, *n2);
}

TEST(reparent, simple) {
    CFG cfg{};
    BasicBlock * n0{cfg.head()};
    BasicBlock * n1{cfg.next()};
    BasicBlock * n2{cfg.next()};
    link_blocks(n0, n1);
    link_blocks(n1, n2);

    BasicBlock * n3{cfg.next()};
    reparent(n0, n3);
    EXPECT_FALSE(n0->left_successor());
    EXPECT_EQ(n3->left_successor(), n1);
    EXPECT_TRUE(n1->predecessors.find(n0) == n1->predecessors.end());
    EXPECT_TRUE(n1->predecessors.find(n3) != n1->predecessors.end());
}

TEST(Node, depth_simple) {
    CFG cfg{};
    BasicBlock * n0{cfg.head()};
    BasicBlock * n1{cfg.next()};
    BasicBlock * n2{cfg.next()};

    link_blocks(n0, n1);
    link_blocks(n1, n2);

    BasicBlock * n3{cfg.next()};
    link_blocks(n2, n3);

    EXPECT_EQ(n0->depth, 0);
    EXPECT_EQ(n1->depth, 1);
    EXPECT_EQ(n2->depth, 2);
    EXPECT_EQ(n3->depth, 3);
}

TEST(Node, depth_no_change) {
    CFG cfg{};
    BasicBlock * n0{cfg.head()};
    BasicBlock * n1{cfg.next()};
    BasicBlock * n2{cfg.next()};

    link_blocks(n0, n1);
    link_blocks(n1, n2);

    BasicBlock * n3{cfg.next()};
    BasicBlock * n4{cfg.next()};
    BasicBlock * n5{cfg.next()};
    BasicBlock * n6{cfg.next()};
    link_blocks(n1, n3, true);
    link_blocks(n3, n4);
    link_blocks(n4, n5);
    link_blocks(n5, n6);

    EXPECT_EQ(n0->depth, 0);
    EXPECT_EQ(n1->depth, 1);
    EXPECT_EQ(n2->depth, 2);
    EXPECT_EQ(n3->depth, 2);
    EXPECT_EQ(n4->depth, 3);
    EXPECT_EQ(n5->depth, 4);
    EXPECT_EQ(n6->depth, 5);

    link_blocks(n2, n6);
    EXPECT_EQ(n6->depth, 5);
}

TEST(Node, depth_deep) {
    CFG cfg{};
    BasicBlock * n0{cfg.head()};
    BasicBlock * n1{cfg.next()};
    BasicBlock * n2{cfg.next()};

    link_blocks(n0, n1);
    link_blocks(n1, n2);

    BasicBlock * n3{cfg.next()};
    BasicBlock * n4{cfg.next()};
    BasicBlock * n5{cfg.next()};
    BasicBlock * n6{cfg.next()};
    link_blocks(n1, n3, true);
    link_blocks(n3, n4);
    link_blocks(n4, n5);
    link_blocks(n5, n6);

    EXPECT_EQ(n0->depth, 0);
    EXPECT_EQ(n1->depth, 1);
    EXPECT_EQ(n2->depth, 2);
    EXPECT_EQ(n3->depth, 2);
    EXPECT_EQ(n4->depth, 3);
    EXPECT_EQ(n5->depth, 4);
    EXPECT_EQ(n6->depth, 5);

    link_blocks(n6, n2);
    // Should not have changed
    EXPECT_EQ(n0->depth, 0);
    EXPECT_EQ(n3->depth, 2);
    EXPECT_EQ(n4->depth, 3);
    EXPECT_EQ(n5->depth, 4);
    EXPECT_EQ(n6->depth, 5);

    // Should have changed
    EXPECT_EQ(n2->depth, 6);
}

TEST(Node, depth_loop_with_break) {
    /*
     *           O preamble
     *           |
     *           O header
     *          / \
     *    body O   |
     *          \ /
     *           O tail
     */
    CFG cfg{};
    BasicBlock * preamble{cfg.head()};

    BasicBlock * header{cfg.next()};
    header->loop_header = true;
    link_blocks(preamble, header);

    BasicBlock * tail{cfg.next()};
    link_blocks(header, tail, true);

    BasicBlock * body{cfg.next()};
    link_blocks(header, body);
    link_blocks(body, header);
    link_blocks(body, tail, true);

    EXPECT_EQ(preamble->depth, 0);
    EXPECT_EQ(header->depth, 1);
    EXPECT_EQ(body->depth, 2);
    EXPECT_EQ(tail->depth, 3);
}
