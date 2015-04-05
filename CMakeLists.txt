cmake_minimum_required(VERSION 3.2.1)

project(MaSzyna LANGUAGES C CXX)

find_package(OpenGL REQUIRED)

set(MASZYNA_SRCS
    AdvSound.h
    AirCoupler.h
    AnimModel.h
    Button.h
    Camera.h
    Classes.h
    Curve.h
    Data.h
    Driver.h
    dumb3d.h
    DynObj.h
    Event.h
    EvLaunch.h
    FadeSound.h
    Float3d.h
    Gauge.h
    Geom.h
    geometry.h
    Globals.h
    Ground.h
    Logs.h
    Machajka.h
    MdlMngr.h
    MemCell.h
    Model3d.h
    Mover.h
    Names.h
    opengl/glew.h
    opengl/glext.h
    opengl/wglew.h
    opengl/wglext.h
    parser.h
    RealSound.h
    ResourceManager.h
    Segment.h
    sky.h
    Sound.h
    Spline.h
    Spring.h
    Texture.h
    TextureDDS.h
    Timer.h
    Track.h
    Traction.h
    TractionPower.h
    Train.h
    TrkFoll.h
    usefull.h
    VBO.h
    wavread.h
    World.h
    AdvSound.cpp
    AirCoupler.cpp
    AnimModel.cpp
    Button.cpp
    Camera.cpp
    Curve.cpp
    Driver.cpp
    dumb3d.cpp
    DynObj.cpp
    EU07.cpp
    Event.cpp
    EvLaunch.cpp
    FadeSound.cpp
    Float3d.cpp
    Gauge.cpp
    Geom.cpp
    geometry.cpp
    Globals.cpp
    Ground.cpp
    Logs.cpp
    Machajka.cpp
    MdlMngr.cpp
    MemCell.cpp
    Model3d.cpp
    Mover.cpp
    Names.cpp
    parser.cpp
    RealSound.cpp
    ResourceManager.cpp
    Segment.cpp
    sky.cpp
    Sound.cpp
    Spline.cpp
    Spring.cpp
    Texture.cpp
    TextureDDS.cpp
    Timer.cpp
    Track.cpp
    Traction.cpp
    TractionPower.cpp
    Train.cpp
    TrkFoll.cpp
    VBO.cpp
    wavread.cpp
    World.cpp
)


add_executable(eu07 ${MASZYNA_SRCS})

set_target_properties(eu07 PROPERTIES 
    CXX_STANDARD 11 
    CXX_EXTENSIONS OFF 
    CXX_STANDARD_REQUIRED ON)

target_include_directories(eu07 PRIVATE ${OPENGL_INCLUDE_DIR})
target_link_libraries(eu07 ${OPENGL_LIBRARIES})
