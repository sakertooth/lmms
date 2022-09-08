INCLUDE(BuildPlugin)
BUILD_PLUGIN(slicer
    SlicerInstrument.cpp
    SlicerView.cpp
    SlicerWaveform.cpp
    SlicerInstrument.h
    SlicerView.h
    SlicerWaveform.h
    MOCFILES
    SlicerInstrument.h
    SlicerView.h
    EMBEDDED_RESOURCES *.png
)