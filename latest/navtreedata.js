/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "Gaia-ECS", "index.html", [
    [ "Table of Contents", "index.html#table-of-contents", null ],
    [ "Introduction", "index.html#introduction", [
      [ "ECS", "index.html#ecs", null ],
      [ "Implementation", "index.html#implementation", null ],
      [ "Project structure", "index.html#project-structure", null ]
    ] ],
    [ "Usage", "index.html#usage", [
      [ "Minimum requirements", "index.html#minimum-requirements", null ],
      [ "Basic operations", "index.html#basic-operations", [
        [ "Create or delete entity", "index.html#create-or-delete-entity", null ],
        [ "Name entity", "index.html#name-entity", null ],
        [ "Names and lookup", "index.html#names-and-lookup", null ],
        [ "Component scope", "index.html#component-scope", null ],
        [ "Storage modes and non-fragmenting membership", "index.html#storage-modes-and-non-fragmenting-membership", [
          [ "Selecting a storage mode", "index.html#selecting-a-storage-mode", null ],
          [ "Non-fragmenting membership", "index.html#non-fragmenting-membership", null ]
        ] ],
        [ "Component presence", "index.html#component-presence", null ],
        [ "Add or remove component", "index.html#add-or-remove-component", null ],
        [ "Component hooks", "index.html#component-hooks", null ],
        [ "Observers", "index.html#observers", [
          [ "Observers for relation pairs", "index.html#observers-for-relation-pairs", null ]
        ] ],
        [ "Bulk editing", "index.html#bulk-editing", null ],
        [ "Set or get component value", "index.html#set-or-get-component-value", null ],
        [ "Copy entity", "index.html#copy-entity", null ],
        [ "Entity cleanup", "index.html#entity-cleanup", null ],
        [ "Batched creation", "index.html#batched-creation", null ],
        [ "Entity lifespan", "index.html#entity-lifespan", [
          [ "SafeEntity", "index.html#safeentity", null ],
          [ "WeakEntity", "index.html#weakentity", null ]
        ] ],
        [ "Archetype lifespan", "index.html#archetype-lifespan", null ]
      ] ],
      [ "Data processing", "index.html#data-processing", [
        [ "Query", "index.html#query", null ],
        [ "Simple query", "index.html#simple-query", null ],
        [ "Query traversal", "index.html#query-traversal", null ],
        [ "Traversal order", "index.html#traversal-order", null ],
        [ "Query variables", "index.html#query-variables", null ],
        [ "Multi-variable queries", "index.html#multi-variable-queries", null ],
        [ "Query low-level API", "index.html#query-low-level-api", null ],
        [ "Query string", "index.html#query-string", null ],
        [ "Uncached query", "index.html#uncached-query", null ],
        [ "Query remarks", "index.html#query-remarks", [
          [ "Query cache behavior", "index.html#query-cache-behavior", null ]
        ] ],
        [ "Iteration", "index.html#iteration", null ],
        [ "Iterate ids", "index.html#iterate-ids", null ],
        [ "Constraints", "index.html#constraints", null ],
        [ "Change detection", "index.html#change-detection", null ],
        [ "Grouping", "index.html#grouping", null ],
        [ "Sorting", "index.html#sorting", null ],
        [ "Parallel execution", "index.html#parallel-execution", null ]
      ] ],
      [ "Relationships", "index.html#relationships", [
        [ "Relationship basics", "index.html#relationship-basics", null ],
        [ "Targets", "index.html#targets", null ],
        [ "Relations", "index.html#relations", null ],
        [ "Entity dependencies", "index.html#entity-dependencies", null ],
        [ "Combination constraints", "index.html#combination-constraints", null ],
        [ "Exclusivity", "index.html#exclusivity", null ],
        [ "Entity inheritance", "index.html#entity-inheritance", null ],
        [ "Prefabs", "index.html#prefabs", null ],
        [ "Cleanup rules", "index.html#cleanup-rules", null ],
        [ "Hierarchies", "index.html#hierarchies", null ]
      ] ],
      [ "Shared data", "index.html#shared-data", null ],
      [ "Delayed execution", "index.html#delayed-execution", [
        [ "Command Merging rules", "index.html#command-merging-rules", [
          [ "Entity Merging", "index.html#entity-merging", null ],
          [ "Component Merging", "index.html#component-merging", null ]
        ] ]
      ] ],
      [ "Systems", "index.html#systems", [
        [ "System basics", "index.html#system-basics", null ],
        [ "System dependencies", "index.html#system-dependencies", null ],
        [ "System jobs", "index.html#system-jobs", null ],
        [ "System callbacks and command buffers", "index.html#system-callbacks-and-command-buffers", null ]
      ] ],
      [ "Data layouts", "index.html#data-layouts", null ],
      [ "Serialization", "index.html#serialization", [
        [ "Compile-time serialization", "index.html#compile-time-serialization", null ],
        [ "Runtime serialization", "index.html#runtime-serialization", null ],
        [ "World serialization", "index.html#world-serialization", null ]
      ] ],
      [ "Runtime components", "index.html#runtime-components", [
        [ "Registration", "index.html#registration", null ],
        [ "Field metadata", "index.html#field-metadata", [
          [ "Optional tool metadata", "index.html#optional-tool-metadata", null ]
        ] ],
        [ "Typed runtime schemas", "index.html#typed-runtime-schemas", null ],
        [ "Schema manifests and validated edits", "index.html#schema-manifests-and-validated-edits", null ],
        [ "Nested structs and fixed arrays", "index.html#nested-structs-and-fixed-arrays", null ],
        [ "Opaque adapters", "index.html#opaque-adapters", null ],
        [ "Dynamic vectors", "index.html#dynamic-vectors", null ],
        [ "Enum and bitmask metadata", "index.html#enum-and-bitmask-metadata", null ],
        [ "Raw access and cursors", "index.html#raw-access-and-cursors", null ],
        [ "Data on runtime relationships", "index.html#data-on-runtime-relationships", null ],
        [ "Querying runtime components", "index.html#querying-runtime-components", null ]
      ] ],
      [ "Containers", "index.html#containers", null ],
      [ "Multithreading", "index.html#multithreading", [
        [ "Worlds, threads, and allocation arenas", "index.html#worlds-threads-and-allocation-arenas", null ],
        [ "Jobs", "index.html#jobs", null ],
        [ "Job dependencies", "index.html#job-dependencies", null ],
        [ "Priorities", "index.html#priorities", null ],
        [ "Job behavior", "index.html#job-behavior", null ],
        [ "Background jobs", "index.html#background-jobs", null ],
        [ "Threads", "index.html#threads", null ],
        [ "Scheduler adapters", "index.html#scheduler-adapters", null ]
      ] ],
      [ "Customization", "index.html#customization", [
        [ "Logging", "index.html#logging", null ]
      ] ]
    ] ],
    [ "Requirements", "index.html#requirements", [
      [ "Compiler", "index.html#compiler", null ],
      [ "Dependencies", "index.html#dependencies", null ]
    ] ],
    [ "Installation", "index.html#installation", [
      [ "CMake", "index.html#cmake", [
        [ "Project settings", "index.html#project-settings", null ],
        [ "Asserts", "index.html#asserts", null ],
        [ "Debug builds", "index.html#debug-builds", null ],
        [ "Sanitizers", "index.html#sanitizers", null ],
        [ "Single-header", "index.html#single-header", null ]
      ] ],
      [ "Conan", "index.html#conan", null ]
    ] ],
    [ "Repository structure", "index.html#repository-structure", [
      [ "Examples", "index.html#examples", null ],
      [ "Benchmarks", "index.html#benchmarks", null ],
      [ "Profiling", "index.html#profiling", null ],
      [ "Testing", "index.html#testing", null ],
      [ "Documentation", "index.html#documentation", null ]
    ] ],
    [ "Future", "index.html#future", null ],
    [ "Contributions", "index.html#contributions", null ],
    [ "License", "index.html#license", null ],
    [ "Namespaces", "namespaces.html", [
      [ "Namespace List", "namespaces.html", "namespaces_dup" ],
      [ "Namespace Members", "namespacemembers.html", [
        [ "All", "namespacemembers.html", "namespacemembers_dup" ],
        [ "Functions", "namespacemembers_func.html", "namespacemembers_func" ],
        [ "Variables", "namespacemembers_vars.html", null ],
        [ "Typedefs", "namespacemembers_type.html", null ],
        [ "Enumerations", "namespacemembers_enum.html", null ],
        [ "Enumerator", "namespacemembers_eval.html", null ]
      ] ]
    ] ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Hierarchy", "hierarchy.html", "hierarchy" ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", "functions_dup" ],
        [ "Functions", "functions_func.html", "functions_func" ],
        [ "Variables", "functions_vars.html", "functions_vars" ],
        [ "Typedefs", "functions_type.html", "functions_type" ],
        [ "Enumerations", "functions_enum.html", null ],
        [ "Enumerator", "functions_eval.html", null ],
        [ "Related Symbols", "functions_rela.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"annotated.html",
"classgaia_1_1cnt_1_1darr__ext.html#a0f5743ed43b85e3965c6f5d28c556658",
"classgaia_1_1cnt_1_1darr__ext__soa.html#aeec6b64b78cd2521a2951fd4310d7153",
"classgaia_1_1cnt_1_1page__storage.html#a37933bef66fbfad97c342c333044f038",
"classgaia_1_1cnt_1_1sarr__ext.html#a4501943afaf7f3d0252764ae008c7a76",
"classgaia_1_1cnt_1_1sarr__soa.html#a5a4091d7715ad8c2e8d18a3adf9e3053",
"classgaia_1_1cnt_1_1sringbuffer.html#a24bbb2b6a8701ee0d4d9b75c36213bd2",
"classgaia_1_1ecs_1_1Archetype.html#a41b9c90757df2d308acbfc616d848d24",
"classgaia_1_1ecs_1_1Chunk.html#aa5dc5d3e0895c877baf2b267b9dcb618",
"classgaia_1_1ecs_1_1QueryInfo.html#a48ae38116f7eb441ff97ff0fd2ef4467",
"classgaia_1_1ecs_1_1World.html#a20fc4b3bad2ca1d55175c6345db01091",
"classgaia_1_1ecs_1_1World.html#a917d386737e94a6eba3005c62090df9e",
"classgaia_1_1ecs_1_1detail_1_1CommandBuffer.html#a26b8c93c2eea8fcf964bad120da98eb4",
"classgaia_1_1ecs_1_1detail_1_1QueryImpl.html#abc73b07a260a753073757d6ce0418d62",
"classgaia_1_1mt_1_1ThreadPool.html#a3b0a7d7790d17f4e46595805c623104d",
"functions_d.html",
"namespacegaia_1_1cnt.html#a2f9bbaff1c172b1dcb7cbc28a91de823",
"namespacegaia_1_1ecs.html#a7d57f54e249a6ee80d63845a9a3e7a5eab1c94ca2fbc3e78fc30069c8d0f01680",
"namespacegaia_1_1ser.html#a0079e1ecf080d80bf1b71ede15340659",
"structgaia_1_1cnt_1_1const__sparse__iterator.html#a54b5fec8fd5c00cc0a18ae075e8a2eb1",
"structgaia_1_1cnt_1_1ilist__handle__traits_3_01TItemHandle_00_01std_1_1void__t_3_01decltype_07st2a0d0d6f9c6aac50cc62708895916597.html",
"structgaia_1_1cnt_1_1sparse__iterator.html#acc715fea6ec6cb6b53e0979368429a98",
"structgaia_1_1core_1_1type__list.html#acdabd7a40b4d3e48740c0f4c308a1f1f",
"structgaia_1_1ecs_1_1ComponentDesc.html#af2899253209ea56f645fcd5cb5f30eca",
"structgaia_1_1ecs_1_1QueryAccessSet.html#a4ef94b5dd47a1809342daa6bb905a357",
"structgaia_1_1ecs_1_1QueryCtx_1_1Data.html#abaddfd1ecccf57175d487cf2ce4c0837",
"structgaia_1_1ecs_1_1QueryTermOptions.html#acffff38b92ada4654faa1c7cf063cb61",
"structgaia_1_1ecs_1_1World_1_1EntityBuilder.html#a5e758f62ad12d8d972113f2969b529ec",
"structgaia_1_1ecs_1_1detail_1_1SystemScheduleScratch.html#a7a136b3950a2491f264db290d1ea04d5",
"structgaia_1_1mem_1_1data__layout__properties_3_01DataLayout_1_1SoA_00_01TItem_01_4.html#a92e38f579f38bcc4db148daf86e7b021",
"structgaia_1_1mt_1_1JobHandle.html#a312daf0e7dcf9b293cde0168d1246d6a",
"structgaia_1_1util_1_1str__view.html#a613609bb6fbd4090504bf5c48c0d0b2d"
];

var SYNCONMSG = 'click to disable panel synchronization';
var SYNCOFFMSG = 'click to enable panel synchronization';
var LISTOFALLMEMBERS = 'List of all members';