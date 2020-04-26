#!/bin/sh

glslc vertex.vert -o vertex.spv
glslc fragment.frag -o fragment.spv
glslc imgui_vertex.vert -o imgui_vertex.spv
glslc imgui_fragment.frag -o imgui_fragment.spv