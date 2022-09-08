INCLUDE(BuildPlugin)
BUILD_PLUGIN(slicer 
    SlicerInstrument.cpp SlicerInstrument.h
    SlicerView.cpp SlicerView.h
    MOCFILES SlicerInstrument.h SlicerView.h
    EMBEDDED_RESOURCES *.png
)