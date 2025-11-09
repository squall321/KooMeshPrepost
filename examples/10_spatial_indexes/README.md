# Spatial Index Examples

This directory contains comprehensive examples demonstrating the spatial index functionality in KooMeshPrepost.

## Overview

The spatial index system provides efficient spatial queries on mesh data using various data structures:
- **Octree**: Hierarchical 8-way subdivision
- **R-Tree**: Bounding volume hierarchy with STR bulk loading
- **K-d Tree**: Binary space partitioning
- **Uniform Grid**: Cell-based spatial hashing

## Files

### spatial_index_demo.cpp
Main demonstration program showcasing all spatial index features.

## Demos Included

### Demo 1: Basic Spatial Index Usage
- Creating and building an index
- Performing bounding box queries
- Finding nearest neighbors
- Viewing index statistics

### Demo 2: Comparing Index Types
- Creating different index types
- Comparing query results
- Understanding performance differences

### Demo 3: Automatic Index Selection
- Using the factory's automatic selection
- Letting the system choose optimal index
- Understanding selection heuristics

### Demo 4: Query Optimizer with Caching
- Using the query optimizer
- Enabling result caching
- Analyzing query patterns
- Getting optimization recommendations

### Demo 5: Performance Benchmarking
- Running comprehensive benchmarks
- Comparing different index types
- Measuring build and query performance
- Understanding speedup metrics

### Demo 6: Generate Performance Documentation
- Running benchmark suite
- Generating documentation in multiple formats
- Creating Markdown reports
- Creating HTML reports
- Exporting JSON data

## Building the Examples

```bash
cd build
cmake ..
make spatial_index_demo
```

## Running the Demo

```bash
./bin/spatial_index_demo
```

## Expected Output

The demo will:
1. Create a 10x10x10 test mesh (1000 elements)
2. Demonstrate basic index operations
3. Compare performance of different index types
4. Show automatic index selection
5. Demonstrate query optimization with caching
6. Run comprehensive benchmarks
7. Generate performance documentation files

## Generated Files

After running the demo, you'll find:
- `spatial_index_performance.md` - Markdown performance report
- `spatial_index_performance.html` - HTML performance report
- `spatial_index_performance.json` - JSON performance data

## Key Concepts Demonstrated

### Spatial Index Types

**Octree**
- Best for: General purpose, hierarchical operations
- Characteristics: 8-way subdivision, good balance
- Use when: Balanced workload, medium-sized meshes

**R-Tree**
- Best for: Range queries, irregular meshes
- Characteristics: STR bulk loading, minimizes overlap
- Use when: Many bounding box queries

**K-d Tree**
- Best for: Nearest neighbor queries, elongated meshes
- Characteristics: Binary partitioning, cycle through axes
- Use when: Point queries, NN searches dominate

**Uniform Grid**
- Best for: Point queries, large uniform meshes
- Characteristics: Cell-based hashing, O(1) average
- Use when: Very large meshes, point queries

### Query Operations

1. **Bounding Box Query**
   ```cpp
   BoundingBox box(minPoint, maxPoint);
   auto results = index->query(box);
   ```

2. **Point Query**
   ```cpp
   auto results = index->queryPoint(point);
   ```

3. **Nearest Neighbor**
   ```cpp
   ElementId nearest = index->findNearest(point);
   ```

4. **K-Nearest Neighbor**
   ```cpp
   auto neighbors = index->findKNearest(point, k);
   ```

5. **Radius Search**
   ```cpp
   auto results = index->findWithinRadius(point, radius);
   ```

### Factory Pattern Usage

```cpp
// Create specific type
auto octree = SpatialIndexFactory::create(SpatialIndexType::OCTREE);

// Create with configuration
SpatialIndexConfig config;
config.type = SpatialIndexType::RTREE;
config.rtreeMaxChildren = 16;
auto rtree = SpatialIndexFactory::create(config);

// Automatic selection
auto optimal = SpatialIndexFactory::createOptimal(mesh);

// Create from string
auto index = SpatialIndexFactory::createFromName("kdtree");
```

### Query Optimization

```cpp
SpatialQueryOptimizer optimizer;
optimizer.setMesh(mesh);

// Enable caching
optimizer.enableCache(1000, 60.0);  // 1000 entries, 60s TTL

// Perform queries (automatically cached)
auto results = optimizer.queryBoundingBox(box);

// Get recommendations
auto recommended = optimizer.recommendIndexType();

// Adapt to workload
optimizer.adaptIndexType();
```

### Performance Benchmarking

```cpp
SpatialIndexBenchmark benchmark;

std::vector<SpatialIndexType> types = { /* ... */ };

// Benchmark operations
auto buildResults = benchmark.benchmarkBuildTime(mesh, types);
auto queryResults = benchmark.benchmarkBoundingBoxQuery(mesh, types, box);
auto nnResults = benchmark.benchmarkNearestNeighbor(mesh, types, point);

// Print comparison
SpatialIndexBenchmark::printComparison(buildResults);

// Export to CSV
SpatialIndexBenchmark::exportToCSV(buildResults, "results.csv");
```

### Documentation Generation

```cpp
PerformanceDocGenerator generator;
generator.setSystemInfo("Ubuntu 22.04", "Intel i7", "32GB");

// Generate reports
std::string markdown = generator.generateMarkdown(results);
std::string html = generator.generateHTML(results);
std::string json = generator.generateJSON(results);

// Save to files
generator.saveToFile(markdown, "report.md");
generator.saveToFile(html, "report.html");
generator.saveToFile(json, "report.json");
```

## Performance Tips

1. **Choose the right index for your workload**
   - Point queries → Uniform Grid
   - Nearest neighbor → K-d Tree
   - Range queries → R-Tree
   - Mixed workload → Octree

2. **Use the query optimizer for adaptive performance**
   - Tracks query patterns
   - Automatically selects optimal index
   - Caches frequent queries

3. **Configure index parameters**
   - Octree: max depth, max elements per node
   - R-Tree: max children per node
   - K-d Tree: max depth, max elements per leaf
   - Uniform Grid: cell size or cell count

4. **Enable caching for repeated queries**
   - Significant speedup for duplicate queries
   - Configurable cache size and TTL
   - Automatic eviction when full

## Further Reading

- See `include/core/ISpatialIndex.h` for the interface
- See `include/core/SpatialIndexFactory.h` for factory details
- See `include/core/SpatialQueryOptimizer.h` for optimization features
- See `include/core/SpatialIndexBenchmark.h` for benchmarking
- See `include/core/PerformanceDocGenerator.h` for documentation

## Notes

- All spatial indexes implement the `ISpatialIndex` interface
- Indexes must be built before querying
- Query results are element IDs that can be used to access mesh elements
- Different indexes may return results in different orders
- Performance characteristics depend on mesh structure and query patterns
