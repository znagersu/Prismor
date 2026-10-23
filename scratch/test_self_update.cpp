#include <iostream>
#include <JuceHeader.h>
#include <windows.h>

int main()
{
    std::cout << "Testing Self-Update mechanism..." << std::endl;
    juce::File currentApp = juce::File::getSpecialLocation(juce::File::currentApplicationFile);
    juce::File currentExe = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    std::cout << "currentApplicationFile: " << currentApp.getFullPathName().toStdString() << std::endl;
    std::cout << "currentExecutableFile:  " << currentExe.getFullPathName().toStdString() << std::endl;

    // Test Windows rename while running
    juce::File bakFile = currentApp.withFileExtension("exe.testbak");
    BOOL renamed = MoveFileExW(currentApp.getFullPathName().toWideCharPointer(),
                              bakFile.getFullPathName().toWideCharPointer(),
                              MOVEFILE_REPLACE_EXISTING);
    std::cout << "Rename active executable: " << (renamed ? "SUCCESS" : "FAILED") << std::endl;
    if (renamed)
    {
        // Restore name
        MoveFileExW(bakFile.getFullPathName().toWideCharPointer(),
                   currentApp.getFullPathName().toWideCharPointer(),
                   MOVEFILE_REPLACE_EXISTING);
        std::cout << "Restored executable name successfully!" << std::endl;
    }

    return 0;
}
