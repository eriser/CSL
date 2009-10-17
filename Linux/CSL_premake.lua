
project.name = "CSL"
project.bindir = "build"
project.libdir = "build"

project.configs = { "Debug", "Release" }

package = newpackage()
package.name = "CSL_JUCE"
package.kind = "winexe"
package.language = "c++"

package.objdir = "build/intermediate"
package.config["Debug"].objdir   = "build/intermediate/Debug"
package.config["Release"].objdir = "build/intermediate/Release"

package.config["Debug"].defines     = { "LINUX=1", "USE_JUCE", "USE_FFTREAL", "USE_HRTF", "USE_JSND" };
package.config["Debug"].buildoptions = { "-ggdb", "-O0", "-w" }

package.config["Release"].defines   = { "LINUX=1", "USE_JUCE", "USE_FFTREAL", "USE_HRTF", "USE_JSND" };
package.config["Release"].buildoptions = { "-O2", "-w" }

package.target = "CSL_JUCE"

package.includepaths = { 
    "../CSL/Includes",
    "../CSL/Spatializers/Binaural",
    "../CSL/Spatializers/Ambisonic",
    "../CSL/Spatializers/VBAP",
	"../JUCE",
    "../../juce/"
}

package.libpaths = { 
    "/usr/X11R6/lib/",
    "/usr/local/lib/",
    "../../juce/bin/"
}

package.config["Debug"].links = { 
    "freetype", "pthread", "X11", "GL", "GLU", "Xinerama", "asound", "juce_debug"
}

package.config["Release"].links = { 
    "freetype", "pthread", "X11", "GL", "GLU", "Xinerama", "asound", "juce_debug"
}

package.linkflags = { "static-runtime" }

package.files = { matchfiles (
    "../CSL/Includes/*.h",
    "../CSL/Kernel/*.cpp",
    "../CSL/Utilities/*.cpp",
    "../CSL/Sources/*.cpp",
    "../CSL/Processors/*.cpp",
    "../CSL/Instruments/*.cpp",
    "../CSL/IO/SoundFile.cpp",
    "../CSL/IO/SoundFileJ.cpp",
    "../CSL/Tests/*.cpp",
    "../CSL/Spatializers/*.cpp",
    "../CSL/Spatializers/Binaural/*",
    "../CSL/Spatializers/Ambisonic/*",
    "../CSL/Spatializers/VBAP/*",
    "../JUCE/*.cpp"
    )
}
