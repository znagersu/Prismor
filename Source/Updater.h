#pragma once
#include <JuceHeader.h>
#include <functional>
#include <vector>

namespace PrismorUpdate
{

enum class UpdateState
{
    Hidden = 0,
    UpdateAvailable,
    Downloading,
    Extracting,
    InstalledRestartReady,
    InstalledReloadDAW,
    ErrorFallback
};

inline bool isRemoteVersionNewer(juce::String remoteTag, juce::String currentVer)
{
    auto parseVer = [](juce::String s) -> std::vector<int>
    {
        s = s.trim();
        while (s.isNotEmpty() && ! juce::CharacterFunctions::isDigit(s[0]))
            s = s.substring(1);

        std::vector<int> parts;
        juce::StringArray tokens;
        tokens.addTokens(s, ".", "");
        for (const auto& t : tokens)
            parts.push_back(t.getIntValue());
        while (parts.size() < 4)
            parts.push_back(0);
        return parts;
    };

    const auto r = parseVer(remoteTag);
    const auto c = parseVer(currentVer);
    const size_t n = std::max(r.size(), c.size());
    for (size_t i = 0; i < n; ++i)
    {
        const int rv = (i < r.size()) ? r[i] : 0;
        const int cv = (i < c.size()) ? c[i] : 0;
        if (rv > cv) return true;
        if (rv < cv) return false;
    }
    return false;
}

class UpdateCheckerThread : public juce::Thread
{
public:
    explicit UpdateCheckerThread(std::function<void(const juce::String&)> onNewVersionFound)
        : juce::Thread("PrismorUpdateChecker"), callback(std::move(onNewVersionFound))
    {
    }

    ~UpdateCheckerThread() override
    {
        signalThreadShouldExit();
        stopThread(500);
    }

    void run() override
    {
        const juce::String simTag = juce::SystemStats::getEnvironmentVariable("PRISMOR_SIMULATE_UPDATE", {});
        if (simTag.isNotEmpty())
        {
            if (callback && ! threadShouldExit())
            {
                juce::MessageManager::callAsync([cb = callback, simTag] {
                    cb(simTag);
                });
            }
            return;
        }

        if (threadShouldExit())
            return;

        try
        {
            const juce::URL url("https://api.github.com/repos/znagersu/Prismor/releases/latest");
            const auto opt = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                 .withConnectionTimeoutMs(2000)
                                 .withExtraHeaders("User-Agent: Prismor-Update-Checker\r\nAccept: application/vnd.github+json\r\n");

            if (auto stream = url.createInputStream(opt))
            {
                if (threadShouldExit())
                    return;

                const juce::String responseText = stream->readEntireStreamAsString();
                const auto json = juce::JSON::parse(responseText);

                if (json.isObject() && json.hasProperty("tag_name"))
                {
                    const juce::String remoteTag = json["tag_name"].toString();
                    const juce::String currentVer = ProjectInfo::versionString;

                    if (isRemoteVersionNewer(remoteTag, currentVer))
                    {
                        if (callback && ! threadShouldExit())
                        {
                            juce::MessageManager::callAsync([cb = callback, remoteTag] {
                                cb(remoteTag);
                            });
                        }
                    }
                }
            }
        }
        catch (...)
        {
        }
    }

private:
    std::function<void(const juce::String&)> callback;
};

class SelfUpdateWorkerThread : public juce::Thread
{
public:
    using ProgressCallback   = std::function<void(float progressNorm, const juce::String& statusMsg)>;
    using CompletionCallback = std::function<void(bool success, bool isStandalone, const juce::String& errorMsg)>;

    SelfUpdateWorkerThread(bool isStandalone, ProgressCallback onProg, CompletionCallback onDone)
        : juce::Thread("PrismorSelfUpdater"),
          isStandaloneHost(isStandalone),
          progressCb(std::move(onProg)),
          doneCb(std::move(onDone))
    {
    }

    ~SelfUpdateWorkerThread() override
    {
        signalThreadShouldExit();
        stopThread(1500);
    }

    void run() override
    {
        auto reportProgress = [this](float p, const juce::String& msg) {
            if (progressCb && ! threadShouldExit())
            {
                juce::MessageManager::callAsync([cb = progressCb, p, msg] {
                    cb(p, msg);
                });
            }
        };

        auto reportDone = [this](bool success, bool isStandalone, const juce::String& err) {
            if (doneCb && ! threadShouldExit())
            {
                juce::MessageManager::callAsync([cb = doneCb, success, isStandalone, err] {
                    cb(success, isStandalone, err);
                });
            }
        };

        reportProgress(0.05f, "CONNECTING...");

        juce::File tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("PrismorSelfUpdate");
        tempDir.createDirectory();
        juce::File zipFile = tempDir.getChildFile("Prismor_Latest.zip");

        try
        {
            const juce::URL url("https://github.com/znagersu/Prismor/releases/latest/download/Prismor_Latest.zip");
            const auto opt = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                 .withConnectionTimeoutMs(10000)
                                 .withNumRedirectsToFollow(5)
                                 .withExtraHeaders("User-Agent: Prismor-Self-Updater\r\n");

            if (auto in = url.createInputStream(opt))
            {
                zipFile.deleteFile();
                juce::FileOutputStream out(zipFile);
                if (! out.openedOk())
                {
                    reportDone(false, isStandaloneHost, "Could not open temp zip for writing.");
                    return;
                }

                const juce::int64 totalLength = in->getTotalLength();
                juce::int64 bytesRead = 0;
                constexpr int bufSize = 65536;
                std::vector<char> buf(bufSize);

                while (! in->isExhausted() && ! threadShouldExit())
                {
                    const int read = in->read(buf.data(), bufSize);
                    if (read <= 0) break;
                    out.write(buf.data(), static_cast<size_t>(read));
                    bytesRead += read;

                    if (totalLength > 0)
                    {
                        const float p = 0.05f + 0.65f * (static_cast<float>(bytesRead) / static_cast<float>(totalLength));
                        reportProgress(p, "DOWNLOADING...");
                    }
                }
                out.flush();

                if (threadShouldExit())
                    return;
            }
            else
            {
                reportDone(false, isStandaloneHost, "Network connection failed.");
                return;
            }

            reportProgress(0.75f, "EXTRACTING...");
            juce::File extractDir = tempDir.getChildFile("extracted");
            extractDir.deleteRecursively();
            extractDir.createDirectory();

            juce::ZipFile zip(zipFile);
            const auto extractResult = zip.uncompressTo(extractDir);
            if (extractResult.failed())
            {
                reportDone(false, isStandaloneHost, "Extraction failed: " + extractResult.getErrorMessage());
                return;
            }

            reportProgress(0.88f, "INSTALLING...");
            const juce::File currentApp = juce::File::getSpecialLocation(juce::File::currentApplicationFile);

            if (isStandaloneHost)
            {
                juce::Array<juce::File> foundExes;
                extractDir.findChildFiles(foundExes, juce::File::findFiles, true, "*.exe");
                if (foundExes.isEmpty())
                {
                    reportDone(false, true, "New executable not found in archive.");
                    return;
                }

                juce::File newExe = foundExes.getFirst();
                for (const auto& f : foundExes)
                {
                    if (f.getFileName().equalsIgnoreCase(currentApp.getFileName()))
                    {
                        newExe = f;
                        break;
                    }
                }

               #if JUCE_WINDOWS
                const juce::File stagedExe = currentApp.getSiblingFile(currentApp.getFileNameWithoutExtension() + "_new.exe");
                if (newExe.copyFileTo(stagedExe))
                {
                    const juce::File batchFile = tempDir.getChildFile("apply_update.bat");
                    juce::String script;
                    script << "@echo off\r\n"
                           << "ping 127.0.0.1 -n 2 > nul\r\n"
                           << ":retry\r\n"
                           << "move /y \"" << stagedExe.getFullPathName() << "\" \"" << currentApp.getFullPathName() << "\" > nul 2>&1\r\n"
                           << "if errorlevel 1 (\r\n"
                           << "    ping 127.0.0.1 -n 2 > nul\r\n"
                           << "    goto retry\r\n"
                           << ")\r\n"
                           << "start \"\" \"" << currentApp.getFullPathName() << "\"\r\n"
                           << "del \"%~f0\"\r\n";
                    batchFile.replaceWithText(script);
                    reportProgress(1.0f, "RESTART TO APPLY");
                    reportDone(true, true, {});
                    return;
                }
               #endif
                reportDone(false, true, "Failed to stage new executable.");
                return;
            }

            // VST3 Mode
            juce::Array<juce::File> foundVst3s;
            extractDir.findChildFiles(foundVst3s, juce::File::findDirectories, true, "*.vst3");
            if (foundVst3s.isEmpty())
            {
                reportDone(false, false, "Updated VST3 bundle not found in archive.");
                return;
            }

            const juce::File srcVst3 = foundVst3s.getFirst();
           #if JUCE_WINDOWS
            const juce::File devVst3("C:\\Users\\endio\\VST3-Dev\\Prismor.vst3");
            const juce::File sysVst3("C:\\Program Files\\Common Files\\VST3\\Prismor.vst3");
            bool ok = false;
            if (devVst3.getParentDirectory().exists())
                ok |= srcVst3.copyDirectoryTo(devVst3);
            if (sysVst3.getParentDirectory().exists())
                ok |= srcVst3.copyDirectoryTo(sysVst3);

            if (ok)
            {
                reportProgress(1.0f, "RELOAD PLUGIN IN DAW");
                reportDone(true, false, {});
                return;
            }
           #endif
            reportDone(false, false, "Could not overwrite VST3 destination.");
        }
        catch (...)
        {
            reportDone(false, isStandaloneHost, "Unexpected self-updater error.");
        }
    }

private:
    bool isStandaloneHost { false };
    ProgressCallback progressCb;
    CompletionCallback doneCb;
};

} // namespace PrismorUpdate
