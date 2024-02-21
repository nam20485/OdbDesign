async function createRelease({github, context, core}, workspacePath, refName) {

    try {
        const createResponse = await github.rest.repos.createRelease({
            generate_release_notes: true,
            name: process.env.RELEASE_NAME,
            owner: context.repo.owner,
            repo: context.repo.repo,
            prerelease: false,
            tag_name: process.env.RELEASE_TAG,
            body: require('fs').readFileSync(workspacePath + '/' + 'release/release-body.md', 'utf8'),
            target_commitish: refName
        });

        const files =
            [
                { name: 'OdbDesign-Linux-x64.zip', contentType: 'application/zip' },
                { name: 'OdbDesign-Linux-x64.zip.sha256sum', contentType: 'text/plain' },
                { name: 'OdbDesign-Linux-x64.zip.asc', contentType: 'text/plain' },
                { name: 'OdbDesign-Windows-x64.zip', contentType: 'application/zip' },
                { name: 'OdbDesign-Windows-x64.zip.sha256sum', contentType: 'text/plain' },
                { name: 'OdbDesign-Windows-x64.zip.asc', contentType: 'text/plain' },
                { name: 'OdbDesign-MacOS-x64.zip', contentType: 'application/zip' },
                { name: 'OdbDesign-MacOS-x64.zip.sha256sum', contentType: 'text/plain' },
                { name: 'OdbDesign-MacOS-x64.zip.asc', contentType: 'text/plain' }
            ];

        const artifactsPath =  workspacePath + '/' + 'artifacts';

        for (const file of files) {
            const filePath = artifactsPath + '/' + file.name;
            const uploadResponse = await github.rest.repos.uploadReleaseAsset({
                owner: context.repo.owner,
                repo: context.repo.repo,
                release_id: createResponse.data.id,
                name: file.name,
                data: require('fs').readFileSync(filePath),
                headers: {
                    'content-type': file.contentType,
                    'content-length': require('fs').statSync(filePath).size
                }
            });
        }
    } catch (error) {
        core.setFailed(error.message);
    }

    return context.payload.client_payload.value
}

module.exports = createRelease;