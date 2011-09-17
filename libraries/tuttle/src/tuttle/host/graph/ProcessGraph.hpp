#ifndef _TUTTLE_HOST_PROCESSGRAPH_HPP_
#define _TUTTLE_HOST_PROCESSGRAPH_HPP_

#include "ProcessVertex.hpp"
#include "ProcessVertexAtTime.hpp"
#include "ProcessEdge.hpp"
#include "ProcessEdgeAtTime.hpp"

#include "InternalGraph.hpp"

#include <tuttle/host/Graph.hpp>

#include <string>

/**
 * @brief If there is a define PROCESSGRAPH_USE_LINK, we don't create a copy of all nodes and
 * The ProcessGraph only contains link to the node in the original Graph.
 */
#define PROCESSGRAPH_USE_LINK

namespace tuttle {
namespace host {
namespace graph {

/**
 * @brief Created from a user Graph, this class allows you to launch the process.
 *
 * Internally this class use multiple graphs with different representation of the graph.
 * It create a new graph with a vertex for each node at each time.
 * It reorder the nodes to optimise memory usage.
 */
class ProcessGraph
{
public:
	typedef Graph::Node Node; /// @todo tuttle ProcessNode...
	typedef graph::ProcessVertex Vertex;
	typedef graph::ProcessEdge Edge;
	typedef Graph::Attribute Attribute;
	typedef InternalGraph<Vertex, Edge, boost::vecS, boost::vecS> InternalGraphImpl;
	typedef InternalGraph<ProcessVertexAtTime, ProcessEdgeAtTime, boost::vecS, boost::vecS> InternalGraphAtTimeImpl;
#ifdef PROCESSGRAPH_USE_LINK
	typedef std::map<std::string, Node*> NodeMap;
#else
	typedef Graph::NodeMap NodeMap;
#endif
	typedef Graph::InstanceCountMap InstanceCountMap;

public:
	ProcessGraph( Graph& graph, const std::list<std::string>& nodes, const bool continueOnError ); ///@ todo: const Graph, no ?
	~ProcessGraph();

	memory::MemoryCache process( const int tBegin, const int tEnd );

private:
	void relink();

	InternalGraphImpl _graph;
	NodeMap _nodes;
	InstanceCountMap _instanceCount;
	bool _continueOnError;

	static const std::string _outputId;
};

}
}
}

#endif

