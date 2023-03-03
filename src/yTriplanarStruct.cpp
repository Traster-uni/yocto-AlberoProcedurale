//
// scicamaru 24/02/2023
//

#include <yocto/yocto_cli.h>
#include <yocto/yocto_gui.h>
#include <yocto/yocto_math.h>
#include <yocto/yocto_scene.h>
#include <yocto/yocto_sceneio.h>
#include <yocto/yocto_shape.h>
#include <yocto/yocto_trace.h>

#include <iostream>

#include "treeToolSet.cpp"
#include "yocto/yocto_modelio.h"
#include "yTreeTriplanarMap.cpp"

struct branch_triplanar_vertex
	{
		vec3f position;
		vec3f normal;
	};

struct plane
    {
       texture_data texture;
       vec4f normal;
       vec3f center;

    };
