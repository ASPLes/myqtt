# generate certificate .key
openssl genrsa -out localhost-server.key 2048
# generate certificate .csr
echo -e "ES\nMadrid\nMadrid\nASPL\nTI Support\nlocalhost\nsoporte@aspl.es\n\n\n" | openssl req -new -key localhost-server.key -out localhost-server.csr
# sign certificate .crt
openssl x509 -req -days 365 -in localhost-server.csr -signkey localhost-server.key -out localhost-server.crt

# generate certificate .key
openssl genrsa -out test19a-localhost-server.key 2048
# generate certificate .csr
echo -e "ES\nMadrid\nMadrid\nASPL\nTI Support\ntest19a.localhost\nsoporte@aspl.es\n\n\n" | openssl req -new -key test19a-localhost-server.key -out test19a-localhost-server.csr
# sign certificate .crt
openssl x509 -req -days 365 -in test19a-localhost-server.csr -signkey test19a-localhost-server.key -out test19a-localhost-server.crt
