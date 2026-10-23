#include <iostream>
#include <JuceHeader.h>

int main()
{
    std::cout << "Testing juce::URL fetch to GitHub API..." << std::endl;
    juce::URL url("https://api.github.com/repos/znagersu/Prismor/releases/latest");
    auto opt = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                   .withConnectionTimeoutMs(3000)
                   .withExtraHeaders("User-Agent: Prismor-Update-Checker\r\nAccept: application/vnd.github+json\r\n");

    std::unique_ptr<juce::InputStream> stream = url.createInputStream(opt);
    if (stream == nullptr)
    {
        std::cout << "FAILED: stream is null!" << std::endl;
        return 1;
    }

    juce::String resp = stream->readEntireStreamAsString();
    std::cout << "SUCCESS! Response length: " << resp.length() << std::endl;
    std::cout << "First 200 chars:\n" << resp.substring(0, 200).toStdString() << std::endl;

    auto json = juce::JSON::parse(resp);
    if (json.isObject())
    {
        std::cout << "tag_name: " << json["tag_name"].toString().toStdString() << std::endl;
    }
    else
    {
        std::cout << "JSON parse failed!" << std::endl;
    }

    return 0;
}
