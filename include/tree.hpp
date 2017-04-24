//////////////////////////////////////////////////////////////////////////////
//  File: cpp-enum-variant/tree.hpp
//////////////////////////////////////////////////////////////////////////////
//  Copyright 2017 Samuel Sleight
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//////////////////////////////////////////////////////////////////////////////

#ifndef ENUM_TREE_HPP
#define ENUM_TREE_HPP

#include <memory>

#include "enum.hpp"

template<typename T>
class TreeNode {
public:
    using NodeType = std::shared_ptr<TreeNode<T>>;

    TreeNode() = delete;

    TreeNode(T data) : data(data), lhs(Optional<NodeType>::None()), rhs(Optional<NodeType>::None()) {}

    void insert(T data) {
        if(data < this->data) {
            lhs.match(
                [this, data](None) { lhs = Optional<NodeType>::Some(std::make_shared<TreeNode<T>>(data)); },
                [data](NodeType node) { node->insert(data); }
            );
        } else {
            rhs.match(
                [this, data](None) { rhs = Optional<NodeType>::Some(std::make_shared<TreeNode<T>>(data)); },
                [data](NodeType node) { node->insert(data); }
            );
        }
    }

    template<typename F>
    void apply(F f) {
        lhs.match(
            [](None) {},
            [&f](NodeType node) { node->apply(f); }
        );

        f(data);

        rhs.match(
            [](None) {},
            [&f](NodeType node) { node->apply(f); }
        );
    }

    bool contains(const T& t) {
        if(t == data) {
            return true;
        } else {
            return (t < data ? lhs : rhs).match(
                [](None) { return false; },
                [t](NodeType node) { return node->contains(t); }
            );
        }
    }

private:
    T data;
    Optional<NodeType> lhs, rhs;
};

template<typename T>
class Tree {
public:
    using NodeType = std::shared_ptr<TreeNode<T>>;

    Tree() : tree(Optional<NodeType>::None()) {}

    void insert(T data) {
        tree.match(
            [this, data](None) { tree = Optional<NodeType>::Some(std::make_shared<TreeNode<T>>(data)); },
            [data](NodeType node) { node->insert(data); }
        );
    }

    template<typename F>
    void apply(F f) {
        tree.match(
            [](None){},
            [&f](NodeType node) { node->apply(f); }
        );
    }

    bool contains(const T& t) {
        return tree.match(
            [](None) { return false; },
            [t](NodeType node) { return node->contains(t); }
        );
    }

private:
    Optional<NodeType> tree;
};

#endif
