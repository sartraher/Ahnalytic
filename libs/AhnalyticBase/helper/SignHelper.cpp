#include "SignHelper.hpp"

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>

//extern "C"
//{
//#include <ms/applink.c>
//}

#include <fstream>
#include <iostream>
#include <vector>

EVP_PKEY* loadPrivateKey(const std::string& path)
{
  BIO* bio = BIO_new_file(path.c_str(), "rb");
  if (!bio)
  {
    std::cerr << "Failed to open key file\n";
    ERR_print_errors_fp(stderr);
    return nullptr;
  }

  EVP_PKEY* pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
  if (!pkey)
  {
    std::cerr << "Failed to load private key\n";
    ERR_print_errors_fp(stderr);
  }

  BIO_free(bio);
  return pkey;
}

EVP_PKEY* loadPublicKey(const std::string& path)
{
  // Open the file as a BIO
  BIO* bio = BIO_new_file(path.c_str(), "rb");
  if (!bio)
  {
    std::cerr << "Failed to open public key file: " << path << "\n";
    ERR_print_errors_fp(stderr);
    return nullptr;
  }

  // Read the public key (PEM format)
  EVP_PKEY* pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
  if (!pkey)
  {
    std::cerr << "Failed to load public key\n";
    ERR_print_errors_fp(stderr);
  }

  BIO_free(bio);
  return pkey;
}

std::vector<unsigned char> SignHelper::readFile(const std::string& path)
{
  std::ifstream file(path, std::ios::binary);
  return std::vector<unsigned char>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

void SignHelper::signFile(const std::string& tarPath, const std::string& privateKeyPath, const std::string& sigPath)
{
  OpenSSL_add_all_algorithms();

  EVP_PKEY* pkey = loadPrivateKey(privateKeyPath);

  EVP_MD_CTX* ctx = EVP_MD_CTX_new();

  EVP_DigestSignInit(ctx, nullptr, EVP_sha256(), nullptr, pkey);

  auto data = readFile(tarPath);

  EVP_DigestSignUpdate(ctx, data.data(), data.size());

  size_t sigLen = 0;
  EVP_DigestSignFinal(ctx, nullptr, &sigLen);

  std::vector<unsigned char> signature(sigLen);

  EVP_DigestSignFinal(ctx, signature.data(), &sigLen);

  // Write signature file
  std::ofstream sigFile(sigPath, std::ios::binary);
  sigFile.write(reinterpret_cast<char*>(signature.data()), sigLen);

  EVP_MD_CTX_free(ctx);
  EVP_PKEY_free(pkey);
}

bool SignHelper::verifyFile(const std::string& tarPath, const std::string& publicKeyPath, const std::string& sigPath)
{
  OpenSSL_add_all_algorithms();

  EVP_PKEY* pkey = loadPublicKey(publicKeyPath);

  EVP_MD_CTX* ctx = EVP_MD_CTX_new();

  EVP_DigestVerifyInit(ctx, nullptr, EVP_sha256(), nullptr, pkey);

  auto data = readFile(tarPath);
  auto signature = readFile(sigPath);

  EVP_DigestVerifyUpdate(ctx, data.data(), data.size());

  int result = EVP_DigestVerifyFinal(ctx, signature.data(), signature.size());

  EVP_MD_CTX_free(ctx);
  EVP_PKEY_free(pkey);

  return result == 1;
}
