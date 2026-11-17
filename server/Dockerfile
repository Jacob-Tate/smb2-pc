# SMB2 PC netplay relay server.
# Build from the repository root:
#   docker build -f server/Dockerfile -t smb2-server .
# Run (UDP port 7855):
#   docker run -d --name smb2-server --restart unless-stopped -p 7855:7855/udp smb2-server

FROM debian:bookworm-slim AS build
RUN apt-get update \
    && apt-get install -y --no-install-recommends cmake g++ make git ca-certificates \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY include/netplay include/netplay
COPY server server
RUN cmake -S server -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --parallel \
    && strip build/smb2-server build/smb2-server-test

FROM debian:bookworm-slim
# Links the ghcr.io package to the repository.
LABEL org.opencontainers.image.source="https://github.com/Jacob-Tate/smb2-pc" \
      org.opencontainers.image.description="SMB2 PC netplay relay server"
RUN useradd --system --no-create-home smb2
COPY --from=build /src/build/smb2-server /usr/local/bin/smb2-server
# The integration test client ships too, so a deployment can be checked in place:
#   docker exec smb2-server smb2-server-test 127.0.0.1 7855
COPY --from=build /src/build/smb2-server-test /usr/local/bin/smb2-server-test
ENV SMB_SERVER_PORT=7855
USER smb2
EXPOSE 7855/udp
ENTRYPOINT ["smb2-server"]
