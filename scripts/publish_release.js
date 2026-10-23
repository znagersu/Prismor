const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');

const PROJECT_ROOT = path.resolve(__dirname, '..');

function getVersion() {
    const cmakePath = path.join(PROJECT_ROOT, 'CMakeLists.txt');
    if (!fs.existsSync(cmakePath)) {
        throw new Error('CMakeLists.txt not found at ' + cmakePath);
    }
    const content = fs.readFileSync(cmakePath, 'utf8');
    const match = content.match(/project\s*\(\s*Prismor\s+VERSION\s+([0-9.]+)/i);
    if (!match) {
        throw new Error('Could not parse version from CMakeLists.txt');
    }
    return match[1].trim();
}

function findGhCli() {
    const candidates = [
        'gh',
        'C:\\Program Files\\GitHub CLI\\gh.exe',
        path.join(process.env.LOCALAPPDATA || '', 'Programs\\GitHub CLI\\gh.exe')
    ];
    for (const c of candidates) {
        try {
            execSync(`"${c}" --version`, { stdio: 'ignore' });
            return c;
        } catch (_) {}
    }
    throw new Error('GitHub CLI (gh) was not found in PATH or standard installation directories.');
}

function packageArtifacts(version) {
    const standaloneExe = path.join(PROJECT_ROOT, 'build', 'Prismor_artefacts', 'Release', 'Standalone', `Prismor ${version}.exe`);
    const vst3Dir = path.join(PROJECT_ROOT, 'build', 'Prismor_artefacts', 'Release', 'VST3', `Prismor ${version}.vst3`);
    
    if (!fs.existsSync(standaloneExe)) {
        throw new Error(`Standalone executable not found at: ${standaloneExe}`);
    }
    if (!fs.existsSync(vst3Dir)) {
        throw new Error(`VST3 bundle not found at: ${vst3Dir}`);
    }

    const stagingDir = path.join(PROJECT_ROOT, 'build', 'release_staging');
    if (fs.existsSync(stagingDir)) {
        fs.rmSync(stagingDir, { recursive: true, force: true });
    }
    fs.mkdirSync(stagingDir, { recursive: true });

    // Copy standalone executable
    fs.copyFileSync(standaloneExe, path.join(stagingDir, `Prismor ${version}.exe`));

    // Copy VST3 bundle recursively
    execSync(`powershell -NoProfile -Command "Copy-Item -Path '${vst3Dir}' -Destination '${stagingDir}' -Recurse -Force"`, {
        stdio: 'inherit'
    });

    const latestZip = path.join(PROJECT_ROOT, 'Prismor_Latest.zip');
    const versionedZip = path.join(PROJECT_ROOT, `Prismor_v${version}.zip`);

    if (fs.existsSync(latestZip)) fs.unlinkSync(latestZip);
    if (fs.existsSync(versionedZip)) fs.unlinkSync(versionedZip);

    console.log(`Creating ${latestZip}...`);
    execSync(`powershell -NoProfile -Command "Compress-Archive -Path '${stagingDir}/*' -DestinationPath '${latestZip}' -Force"`, {
        stdio: 'inherit'
    });

    console.log(`Creating ${versionedZip}...`);
    fs.copyFileSync(latestZip, versionedZip);

    return { latestZip, versionedZip };
}

function publishRelease(version, latestZip, versionedZip) {
    const gh = findGhCli();
    const tag = `v${version}`;
    const releaseTitle = `Prismor v${version}`;
    const notes = `Automated build release for Prismor v${version}.\n\n` +
                  `### Download Assets\n` +
                  `- **Permanent Latest Download**: [Prismor_Latest.zip](https://github.com/znagersu/Prismor/releases/latest/download/Prismor_Latest.zip)\n` +
                  `- **Release Archive**: [Prismor_v${version}.zip](https://github.com/znagersu/Prismor/releases/download/${tag}/Prismor_v${version}.zip)\n\n` +
                  `Includes Standalone .exe and VST3 64-bit plugin.`;

    console.log(`Checking if release ${tag} exists on GitHub...`);
    let releaseExists = false;
    try {
        execSync(`"${gh}" release view "${tag}" --repo znagersu/Prismor`, { stdio: 'ignore' });
        releaseExists = true;
    } catch (_) {
        releaseExists = false;
    }

    if (releaseExists) {
        console.log(`Release ${tag} exists. Uploading assets with clobber...`);
        execSync(`"${gh}" release upload "${tag}" "${latestZip}" "${versionedZip}" --repo znagersu/Prismor --clobber`, {
            stdio: 'inherit'
        });
        execSync(`"${gh}" release edit "${tag}" --title "${releaseTitle}" --notes "${notes}" --latest --repo znagersu/Prismor`, {
            stdio: 'inherit'
        });
    } else {
        console.log(`Creating new release ${tag}...`);
        execSync(`"${gh}" release create "${tag}" "${latestZip}" "${versionedZip}" --repo znagersu/Prismor --title "${releaseTitle}" --notes "${notes}" --latest`, {
            stdio: 'inherit'
        });
    }

    console.log(`\n=== SUCCESS: Release ${tag} published to GitHub ===`);
    console.log(`Permanent Static Download URL:`);
    console.log(`https://github.com/znagersu/Prismor/releases/latest/download/Prismor_Latest.zip\n`);
}

function main() {
    try {
        console.log('=== Prismor Automated Build-to-Release Pipeline ===');
        const version = getVersion();
        console.log(`Detected Version: ${version}`);

        console.log('Packaging compiled release artifacts...');
        const { latestZip, versionedZip } = packageArtifacts(version);

        console.log('Publishing release to GitHub...');
        publishRelease(version, latestZip, versionedZip);
    } catch (err) {
        console.error('Release failed:', err.message);
        process.exit(1);
    }
}

main();
