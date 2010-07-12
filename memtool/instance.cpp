/*
 * instance.cpp
 *
 *  Created on: 02.07.2010
 *      Author: chrschn
 */

#include "instance.h"
#include "structured.h"
#include "refbasetype.h"
#include "pointer.h"
#include "virtualmemory.h"
#include "debug.h"
#include <QScriptEngine>

Q_DECLARE_METATYPE(Instance)

const QStringList Instance::_emtpyStringList;


Instance::Instance()
	: _address(0),  _type(0), _vmem(0), _isNull(true)
{
}


Instance::Instance(size_t address, const BaseType* type, const QString& name,
		const QString& parentName, VirtualMemory* vmem)
	: _address(address),  _type(type), _name(name), _parentName(parentName),
	  _vmem(vmem), _isNull(true)
{
	_isNull = !_address || !_type || !_vmem;
}


Instance::~Instance()
{
}


quint64 Instance::address() const
{
	return _address;
}


QString Instance::name() const
{
	return _name;
}


QString Instance::parentName() const
{
	return _parentName;
}


QString Instance::fullName() const
{
	if (_parentName.isEmpty())
		return _name;
	else
		return QString("%1.%2").arg(_parentName).arg(_name);
}


int Instance::memberCount() const
{
	const Structured* s = dynamic_cast<const Structured*>(_type);
	return s ? s->members().size() : 0;
}


const QStringList& Instance::memberNames() const
{
	const Structured* s = dynamic_cast<const Structured*>(_type);
	return s ? s->memberNames() : Instance::_emtpyStringList;
}


InstanceList Instance::members() const
{
	const Structured* s = dynamic_cast<const Structured*>(_type);
	if (!s)
		return InstanceList();

	const MemberList& list = s->members();
	InstanceList ret;
	QString fullName = this->fullName();
	for (int i = 0; i < list.count(); ++i)
		ret.append(list[i]->toInstance(_address, _vmem, fullName));
	return ret;
}


const BaseType* Instance::type() const
{
	return _type;
}


QString Instance::typeName() const
{
	return _type ? _type->prettyName() : QString();
}


quint32 Instance::size() const
{
	return _type ? _type->size() : 0;
}


bool Instance::isNull() const
{
	return _isNull;
}


Instance Instance::member(int index) const
{
	const Structured* s = dynamic_cast<const Structured*>(_type);
	if (s && index >= 0 && index < s->members().size())
		return s->members().at(index)->toInstance(_address, _vmem, fullName());
	return Instance();
}


bool Instance::memberExists(const QString& name) const
{
	const Structured* s = dynamic_cast<const Structured*>(_type);
	return s ? s->memberExists(name) : false;
}


Instance Instance::findMember(const QString& name) const
{
	const Structured* s = dynamic_cast<const Structured*>(_type);
	const StructuredMember* m = 0;
	if ( !s || !(m = s->findMember(name)) )
		return Instance();

	return m->toInstance(_address, _vmem, fullName());
}


int Instance::indexOfMember(const QString& name) const
{
	const Structured* s = dynamic_cast<const Structured*>(_type);
	return s ? s->memberNames().indexOf(name) : -1;
}


int Instance::typeIdOfMember(const QString& name) const
{
	const Structured* s = dynamic_cast<const Structured*>(_type);
	const StructuredMember* m = 0;
	if ( !s || !(m = s->findMember(name)) )
		return 0;
	else
		return m->refTypeId();
}


QString Instance::toString() const
{
	return _type ? _type->toString(_vmem, _address) : QString();
}
