#pragma once

#include <memory>
#include <cassert>
#include <iterator>

#include <gumbo.h>

namespace EasyGumbo
{

class DFSIterator: public std::iterator<std::input_iterator_tag, GumboNode>
{
public:

    inline DFSIterator(const DFSIterator &dfsIter) noexcept;
    inline explicit DFSIterator(GumboNode *node = nullptr) noexcept;

    DFSIterator fromCurrent() const noexcept
    {
        return DFSIterator(m_current);
    }

    bool operator!=(const DFSIterator &obj) const noexcept
    {
        return (m_current != obj.m_current);
    }

    bool operator==(const DFSIterator &obj) const noexcept
    {
        return (m_current == obj.m_current);
    }

    DFSIterator &operator++() noexcept;
    DFSIterator operator++(int) noexcept
    {
        DFSIterator tmp(*this);
        operator++();
        return tmp;
    }

    inline bool gotoAdj() noexcept;

    GumboNode *operator->() noexcept
    {
        return m_current;
    }

    GumboNode &operator*() noexcept
    {
        return *m_current;
    }

private:

    inline bool gotoFirstChildren() noexcept;
    inline bool gotoChildren(const GumboVector &vector, size_t indx) noexcept;

private:

    GumboNode *m_start;
    GumboNode *m_current;
};

/*!
 * RAII обертка над Си API
 */
class Gumbo
{
public:

    typedef DFSIterator iterator;

    /*!
     * Создать парсер
     */
    inline Gumbo(const char *buffer);

    /*!
     * Проверка на Null
     */
    operator bool() const
    {
        return m_output != nullptr;
    }

    GumboOutput *output() const
    {
        return m_output.get();
    }

    iterator begin() noexcept
    {
        return iterator(m_output->document);
    }

    iterator end() noexcept
    {
        return iterator();
    }

private:

    std::shared_ptr<GumboOutput> m_output;
};

template<typename T>
struct Vector
{
public:
    typedef T value_type;

    Vector(GumboVector &vector)
        : m_vector(vector)
    {

    }

    value_type operator[](const size_t indx) const noexcept
    {
        assert(indx < m_vector.length);
        return  static_cast<value_type>(m_vector.data[indx]);
    }

    size_t size() const noexcept
    {
        return m_vector.length;
    }

    GumboVector &m_vector;
};

struct Element
{
    typedef Vector<GumboNode*> ChildrenList;


    Element(GumboNode &element) noexcept :
        m_node(element)
    {
        assert(GUMBO_NODE_ELEMENT == m_node.type);
    }

    ChildrenList children() const noexcept
    {
        return ChildrenList(m_node.v.element.children);
    }

    const GumboSourcePosition& start() const noexcept
    {
        return m_node.v.element.start_pos;
    }

    const GumboSourcePosition& end() const noexcept
    {
        return m_node.v.element.end_pos;
    }

    const GumboAttribute* attribute(const char* name ) const noexcept
    {
        return gumbo_get_attribute(&m_node.v.element.attributes, name);
    }

    GumboNode &m_node;
};

struct HasAttribute
{
    HasAttribute(const std::string &attribute,
                 const std::string &value = std::string())
        :
        m_attribute(attribute),
        m_value(value)
    {
    }

    bool operator()(const GumboNode &node) const noexcept
    {
        if (node.type != GUMBO_NODE_ELEMENT) {
            return false;
        }

        const GumboVector *vec = &node.v.element.attributes;
        auto attribute = gumbo_get_attribute(vec, m_attribute.c_str());
        if (attribute != nullptr) {
            if (!m_value.empty()) {
                return m_value == attribute->value;
            }
            else {
                return true;
            }
        }
        return false;
    }

    const std::string m_attribute;
    const std::string m_value;
};

struct Tag
{
    Tag(GumboTag tag)
        :
        m_tag(tag)
    {
    }

    bool operator()(const GumboNode &node) const noexcept
    {
        if (node.type != GUMBO_NODE_ELEMENT) {
            return false;
        }

        return node.v.element.tag == m_tag;
    }

    const GumboTag m_tag;
};

template<typename Comp1, typename Comp2>
struct LogicalAnd
{
    LogicalAnd(const Comp1 &cmp1, const Comp2 &cmp2)
        :
        m_cmp1(cmp1),
        m_cmp2(cmp2)
    {

    }

    bool operator()(const GumboNode &node) const noexcept
    {
        return m_cmp1(node) && m_cmp2(node);
    }

    const Comp1 m_cmp1;
    const Comp2 m_cmp2;
};

template<typename Comp1, typename Comp2>
LogicalAnd<Comp1, Comp2> And(const Comp1 &cmp1, const Comp2 &cmp2)
{
    return LogicalAnd<Comp1, Comp2>(cmp1, cmp2);
};

template<typename Iterator,
    typename UnaryPredicate>
std::vector<GumboNode*> findAll(Iterator begin, Iterator end, UnaryPredicate predicate)
{
    std::vector<GumboNode*> container;
    for(Iterator i(begin); i != end; ++i)
    {
        if(predicate(*i))
        {
            container.push_back(&(*i));
        }
    }
    return container;
}

/***        Implementation        ***/

DFSIterator::DFSIterator(const DFSIterator &dfsIter) noexcept
    :
    m_start(dfsIter.m_start),
    m_current(dfsIter.m_current)
{
}

DFSIterator::DFSIterator(GumboNode *node) noexcept
    :
    m_start(node),
    m_current(node)
{
}

DFSIterator &DFSIterator::operator++() noexcept
{
    if (m_current == nullptr) {
        return *this;
    }

    if (!gotoFirstChildren()) {
        // переходим к смежному узлу дерева
        while (!gotoAdj()) {
            // если не уалось поднимаемся вверх по дереву,
            // вруг есть где-то ветвь
            m_current = m_current->parent;
            if (m_current == m_start) {
                // Завершии обход в глубину
                m_current = nullptr;
                break;
            }
        }
    }
    return *this;
}

bool DFSIterator::gotoFirstChildren() noexcept
{
    assert(m_current != nullptr);

    switch (m_current->type) {
        case GUMBO_NODE_ELEMENT:
            return gotoChildren(m_current->v.element.children, 0);
        case GUMBO_NODE_DOCUMENT:
            return gotoChildren(m_current->v.document.children, 0);
        default:
            return false;
    }
}

bool DFSIterator::gotoChildren(const GumboVector &vector, size_t indx) noexcept
{
    assert(m_current != nullptr);

    if ((vector.length) != 0
        && (indx < vector.length)) {
        m_current = static_cast<GumboNode *>(vector.data[indx]);
        return true;
    }
    return false;
}

bool DFSIterator::gotoAdj() noexcept
{
    if (m_current == m_start
        || m_current == nullptr) {
        return false;
    }

    const GumboNode *parent = m_current->parent;
    const size_t nextIndx = m_current->index_within_parent + 1;

    switch (parent->type) {
        case GUMBO_NODE_ELEMENT:
            return gotoChildren(parent->v.element.children, nextIndx);
        case GUMBO_NODE_DOCUMENT:
            return gotoChildren(parent->v.document.children, nextIndx);
        default:
            return false;
    }
}

Gumbo::Gumbo(const char *buffer)
{
    auto output = gumbo_parse(buffer);
    m_output = std::shared_ptr<GumboOutput>(output, [](GumboOutput *obj)
    {
        gumbo_destroy_output(&kGumboDefaultOptions, obj);
    });
}
}