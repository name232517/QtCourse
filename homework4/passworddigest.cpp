#include "passworddigest.h"

#include <QList>
#include <QPasswordDigestor>
#include <QRandomGenerator>

namespace {
constexpr int DefaultIterations = 600000;
constexpr int MaxIterations = 1200000;
constexpr int SaltSize = 16;
constexpr int DigestSize = 32;
constexpr auto AlgorithmName = "pbkdf2-sha256";

QByteArray derive(const QByteArray &password, const QByteArray &salt, int iterations)
{
    return QPasswordDigestor::deriveKeyPbkdf2(
        QCryptographicHash::Sha256, password, salt, iterations, DigestSize);
}
}

QByteArray PasswordDigest::create(const QByteArray &password)
{
    QByteArray salt(SaltSize, Qt::Uninitialized);
    for (char &byte : salt) {
        byte = static_cast<char>(QRandomGenerator::system()->bounded(256u));
    }

    const QByteArray digest = derive(password, salt, DefaultIterations);
    if (digest.size() != DigestSize) {
        return {};
    }
    return QByteArray(AlgorithmName) + '$' + QByteArray::number(DefaultIterations)
        + '$' + salt.toHex() + '$' + digest.toHex();
}

PasswordDigest::Check PasswordDigest::verify(const QByteArray &password, const QByteArray &encodedDigest)
{
    const auto parts = encodedDigest.split('$');
    if (parts.size() != 4 || parts[0] != AlgorithmName) {
        return Check::Unavailable;
    }

    bool validIterations = false;
    const int iterations = parts[1].toInt(&validIterations);
    const QByteArray salt = QByteArray::fromHex(parts[2]);
    const QByteArray expected = QByteArray::fromHex(parts[3]);
    if (!validIterations || iterations < 1 || iterations > MaxIterations
        || salt.size() != SaltSize || salt.toHex() != parts[2].toLower()
        || expected.size() != DigestSize || expected.toHex() != parts[3].toLower()) {
        return Check::Unavailable;
    }

    const QByteArray actual = derive(password, salt, iterations);
    if (actual.size() != DigestSize) {
        return Check::Unavailable;
    }

    unsigned int difference = 0;
    for (int i = 0; i < DigestSize; ++i) {
        difference |= static_cast<unsigned char>(actual.at(i))
                    ^ static_cast<unsigned char>(expected.at(i));
    }
    return difference == 0 ? Check::Match : Check::Mismatch;
}
