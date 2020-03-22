/*
 * id.h
 * Copyright 2016, Thorbj酶rn Lindeijer <bjorn@lindeijer.nl>
 *
 * This file is part of Tiled.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QLatin1String>

namespace Tiled {
namespace Internal {

class Id
{
public:
    Id(const char *name);

    QByteArray name() const;

    bool operator==(Id id) const { return mId == id.mId; }
    bool operator!=(Id id) const { return mId != id.mId; }

private:
    uint mId;

    friend uint qHash(Id id) Q_DECL_NOTHROW;//修饰的函数表示不会抛出异常,如果抛出了异常，编译器直接调用std::terminate()
};
/**
 * @brief 有元函数可以通过类对象在函数中访问对象的私有成员
 * @param id
 * @return
 */
inline uint qHash(Id id) Q_DECL_NOTHROW
{
    return id.mId;
}

} // namespace Internal
} // namespace Tiled
// 在C++11中Q_DECL(declare)_CONSTEXPR为constexpr 常量表达式
// 在C++11中Q_DECL_NOTHROW为noexcept 函数
//参看qt源代码可以发现，使用QT_NO_DEBUG_STREAM便可以禁用debug，waring，critical信息，但是不会禁用fatal信息，这个需要注意一下。
//https://blog.csdn.net/ieearth/article/details/77009065
