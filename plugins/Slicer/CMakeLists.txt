INCLUDE(BuildPlugin)
BUILD_PLUGIN(slicer
    SlicerInstrument.cpp
    SlicerView.cpp
    SlicerWaveform.cpp
    SlicerWindow.cpp
    SlicerInstrument.h
    SlicerView.h
    SlicerWaveform.h
    SlicerWindow.h
    MOCFILES
    SlicerInstrument.h
    SlicerView.h
    SlicerWindow.h
    EMBEDDED_RESOURCES *.png
)