#pragma once

#include <QByteArray>

namespace PasswordDigest {
enum class Check { Match, Mismatch, Unavailable };

QByteArray create(const QByteArray &password);
Check verify(const QByteArray &password, const QByteArray &encodedDigest);
}
