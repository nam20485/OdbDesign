try {       
    const createResponse = await github.rest.repos.createRelease({
    generate_release_notes: true,
    name: process.env.RELEASE_NAME,
    owner: context.repo.owner,
    repo: context.repo.repo,
    tag_name: process.env.RELEASE_TAG,
    body: require('fs').readFileSync('${{ github.workspace }}/release-body.md', 'utf8'),
    target_commitish: '${{ github.ref_name }}'
  });

  const filenames = [ 
      'OdbDesign-Linux-x64.zip',
      'OdbDesign-Windows-x64.zip',
      'OdbDesign-MacOS-x64.zip'
  ];
  
  for (const filename of filenames) {                  
      const artifactsPath = '${{ github.workspace }}/artifacts';
      const filePath = artifactsPath +'/' + filename;
      const uploadResponse = await github.rest.repos.uploadReleaseAsset({
          owner: context.repo.owner,
          repo: context.repo.repo,
          release_id: createResponse.data.id,        
          name: filename,            
          data: require('fs').readFileSync(filePath),
          headers: {
          'content-type': 'application/zip',
          'content-length': require('fs').statSync(filePath).size
          }    
      });
  }
} catch (error) {
    core.setFailed(error.message);
}   