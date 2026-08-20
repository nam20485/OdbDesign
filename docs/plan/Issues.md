# Issues 

The following issues need to be resolved.

## Issues List

1. has the state of the @scripts/deploy-monitoring.ps1 and the constiuent resource manifests been checked for correctness and viability? 
2. if not, then we need to get it working
3. Fix the @scripts/deploy-monitoring.ps1 script to auto install dependencies on linux  (apt/Debian-based distros) in addition to Windows
4. We need to add capability to have the agent deploy monitoring to the @.agents/skills/k3s-admin/SKILL.md 
5. add a new capability to the @.agents/skills/k3s-admin/SKILL.md : generate openapi spec. It analyzes the REST API interface by inspecting the source code in @OdbDesignServer/ 's REST API and generates a new high quality open api spec that the swaggerui service can use @swagger/odbdesign-server-0.9-swagger.yaml . Make sure it uses all of the openapi specs capcbilites and markup features (like responses and field types and everything it support so that the generate swagger ui is as fancy as is supported)

scripts/deploy.ps1:71-74
```
$specPath = "swagger/odbdesign-server-0.9-swagger.yaml"
    & kubectl create configmap odbdesign-server-swagger-spec `
        --from-file=odbdesign-server-0.9-swagger.yaml=$specPath `
        --dry-run=client -o yaml | kubectl apply -f -
```

deploy/kube/OdbDesignServer-SwaggerUI/deployment.yaml:36-39
```
 volumeMounts:
            - name: swagger-spec
              mountPath: /spec/odbdesign-server-0.9-swagger.yaml
              subPath: odbdesign-server-0.9-swagger.yaml
```

