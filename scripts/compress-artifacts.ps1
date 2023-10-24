

# COPY --from=build /src/OdbDesign/out/build/linux-release/OdbDesignLib/*.so .
# COPY --from=build /src/OdbDesign/out/build/linux-release/Utils/*.so .
# COPY --from=build /src/OdbDesign/out/build/linux-release/OdbDesignServer/OdbDesignServer .

New-Item -ItemType Directory -Force -Path ".\artifacts" -V
Copy-Item -Path ".\out\build\x64-release\*.dll" -Destination ".\artifacts" -Force -V
Copy-Item -Path ".\out\build\x64-release\OdbDesignServer.exe" -Destination ".\artifacts" -Force -V    
# COPY --from=build /src/OdbDesign/out/build/linux-release/OdbDesignLib/*.so .
# COPY --from=build /src/OdbDesign/out/build/linux-release/Utils/*.so .  
Compress-Archive -Path ".\artifacts\*.dll",".\artifacts\*.exe"  -DestinationPath ".\artifacts\artifacts.zip" -V -Force
