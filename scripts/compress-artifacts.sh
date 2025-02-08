#! /bin/sh

# COPY --from=build /src/OdbDesign/out/build/linux-release/OdbDesignLib/*.so .
# COPY --from=build /src/OdbDesign/out/build/linux-release/Utils/*.so .
# COPY --from=build /src/OdbDesign/out/build/linux-release/OdbDesignServer/OdbDesignServer .

mkdir ./artifacts
cp ./out/build/linux-release/OdbDesignLib/*.so ./artifacts
cp ./out/build/linux-release/Utils/*.so ./artifacts
cp ./out/build/linux-release/OdbDesignServer/OdbDesignServer ./artifacts
#cp ./out/build/linux-release/OdbDesignServer/*.so ./artifacts
cd ./artifacts
zip -r ./artifacts.zip ./*.so ./OdbDesignServer