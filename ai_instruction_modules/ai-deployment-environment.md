# Google Cloud Production Deployment Configuration Guide

This guide covers the complete setup process for deploying AgentAsAService to Google Cloud in production. It specfically only applies to that project.

## Prerequisites

1. **Google Cloud CLI installed and authenticated**
   ```bash
   gcloud auth login
   gcloud config set project agent-as-a-service-459620
   ```

2. **Firebase CLI installed and authenticated**
   ```bash
   npm install -g firebase-tools
   firebase login
   ```

3. **Docker installed** (for local testing)

## Service Accounts Setup

Run the service account setup script:

```bash
# Linux/macOS
./scripts/setup-gcp-service-accounts.sh

# Windows PowerShell
./scripts/setup-gcp-service-accounts.ps1
```

This creates:
- `orchestrator@agent-as-a-service-459620.iam.gserviceaccount.com`
- `agent-service@agent-as-a-service-459620.iam.gserviceaccount.com`

## Required Environment Variables

### Cloud Build Trigger Variables

Set these in your Cloud Build trigger configuration:

| Variable | Description | Example |
|----------|-------------|---------|
| `_GOOGLE_CLIENT_ID` | Google OAuth 2.0 Client ID | `123456789-abcdef.apps.googleusercontent.com` |
| `_GITHUB_APP_ID` | GitHub App ID | `123456` |
| `_GITHUB_INSTALLATION_ID` | GitHub App Installation ID | `987654321` |

### Google OAuth 2.0 Setup

1. Go to [Google Cloud Console > APIs & Services > Credentials](https://console.cloud.google.com/apis/credentials)
2. Create OAuth 2.0 Client ID (Web application)
3. Add authorized JavaScript origins:
   - `https://agent-as-a-service-459620.web.app`
   - `https://agent-as-a-service-459620.firebaseapp.com`
4. Add authorized redirect URIs:
   - `https://agent-as-a-service-459620.web.app/authentication/login-callback`
   - `https://agent-as-a-service-459620.firebaseapp.com/authentication/login-callback`

### GitHub App Setup

1. Create a GitHub App in your organization
2. Generate and download the private key
3. Install the app in your repositories
4. Update the secret:
   ```bash
   gcloud secrets versions add github-app-private-key --data-file=path/to/your/github-app-key.pem
   ```

## Deployment Process

### 1. Deploy Backend Services (Cloud Run)

Use Cloud Build:
```bash
gcloud builds submit --config cloudbuild.yaml --substitutions=_TAG_NAME=$(git rev-parse --short HEAD)
```

Or manually:
```bash
# Build and push images
docker build -t us-west1-docker.pkg.dev/agent-as-a-service-459620/agent-as-a-service/orchestratorservice:latest -f OrchestratorService/Dockerfile .
docker build -t us-west1-docker.pkg.dev/agent-as-a-service-459620/agent-as-a-service/agentservice:latest -f AgentService/Dockerfile .

docker push us-west1-docker.pkg.dev/agent-as-a-service-459620/agent-as-a-service/orchestratorservice:latest
docker push us-west1-docker.pkg.dev/agent-as-a-service-459620/agent-as-a-service/agentservice:latest

# Deploy to Cloud Run
gcloud run deploy orchestratorservice \
  --image us-west1-docker.pkg.dev/agent-as-a-service-459620/agent-as-a-service/orchestratorservice:latest \
  --region us-west1 \
  --service-account orchestrator@agent-as-a-service-459620.iam.gserviceaccount.com \
  --set-env-vars ASPNETCORE_ENVIRONMENT=Production,GOOGLE_CLIENT_ID=$GOOGLE_CLIENT_ID

gcloud run deploy agentservice \
  --image us-west1-docker.pkg.dev/agent-as-a-service-459620/agent-as-a-service/agentservice:latest \
  --region us-west1 \
  --service-account agent-service@agent-as-a-service-459620.iam.gserviceaccount.com \
  --set-env-vars ASPNETCORE_ENVIRONMENT=Production,GITHUB_APP_ID=$GITHUB_APP_ID \
  --no-allow-unauthenticated
```

### 2. Deploy Frontend (Firebase Hosting)

```bash
# Linux/macOS
./scripts/deploy-firebase.sh

# Windows PowerShell
./scripts/deploy-firebase.ps1
```

## Service URLs

After deployment, your services will be available at:

- **Blazor WebApp**: `https://agent-as-a-service-459620.web.app`
- **OrchestratorService**: `https://orchestratorservice-us-west1-agent-as-a-service-459620.run.app`
- **AgentService**: `https://agentservice-us-west1-agent-as-a-service-459620.run.app`

## Testing the Deployment

1. **Test Blazor WebApp**:
   - Visit `https://agent-as-a-service-459620.web.app`
   - Try Google sign-in
   - Check browser developer tools for errors

2. **Test OrchestratorService**:
   ```bash
   curl https://orchestratorservice-us-west1-agent-as-a-service-459620.run.app/api/auth/test
   ```

3. **Test AgentService** (requires authentication):
   ```bash
   # This should return 401 Unauthorized (correct behavior)
   curl https://agentservice-us-west1-agent-as-a-service-459620.run.app/api/agent/test
   ```

## Monitoring and Logging

- **Cloud Run Logs**: [Google Cloud Console > Cloud Run](https://console.cloud.google.com/run)
- **Firebase Hosting**: [Firebase Console > Hosting](https://console.firebase.google.com/project/agent-as-a-service-459620/hosting)
- **Firestore**: [Firebase Console > Firestore](https://console.firebase.google.com/project/agent-as-a-service-459620/firestore)

## Troubleshooting

### Authentication Issues

1. **Check Google OAuth 2.0 configuration**:
   - Verify client ID in appsettings.Production.json
   - Check authorized origins and redirect URIs

2. **Check service account permissions**:
   ```bash
   gcloud projects get-iam-policy agent-as-a-service-459620 --flatten="bindings[].members" --filter="bindings.members:orchestrator@*"
   ```

3. **Check Cloud Run environment variables**:
   ```bash
   gcloud run services describe orchestratorservice --region us-west1 --format="export"
   ```

### CORS Issues

If you encounter CORS errors:
1. Check AllowedOrigins in OrchestratorService appsettings.Production.json
2. Verify Firebase Hosting URLs are correctly configured
3. Check browser network tab for preflight requests

### Service-to-Service Authentication

1. **Verify service account key access**:
   ```bash
   gcloud run services describe orchestratorservice --region us-west1 --format="value(spec.template.spec.serviceAccountName)"
   ```

2. **Check JWT token generation**:
   - Look at OrchestratorService logs for authentication attempts
   - Verify AgentService receives and validates tokens correctly

## Security Best Practices

1. **Use least privilege IAM roles**
2. **Enable Cloud Run IAM for AgentService** (no-allow-unauthenticated)
3. **Use Google Cloud Secret Manager** for sensitive configuration
4. **Enable Cloud Logging and Monitoring**
5. **Regularly rotate GitHub App private keys**
6. **Use HTTPS everywhere** (enforced by Cloud Run and Firebase)

## References

- [ASP.NET Core Authentication Documentation](https://docs.microsoft.com/en-us/aspnet/core/security/authentication/)
- [Google Cloud Run Documentation](https://cloud.google.com/run/docs)
- [Firebase Hosting Documentation](https://firebase.google.com/docs/hosting)
- [Google Cloud IAM Best Practices](https://cloud.google.com/iam/docs/using-iam-securely)
