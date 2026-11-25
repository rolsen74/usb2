
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef INC_DESCRIPTOR_FIELDS_H
#define INC_DESCRIPTOR_FIELDS_H

struct DescriptorOutput;

enum DescriptorFormat
{
	DESC_HEX,
	DESC_DECIMAL,
	DESC_BCD,
	DESC_GUID
};

struct DescriptorField
{
	const char *Name;
	U8 Size;
	enum DescriptorFormat Format;
};

struct ClassDescriptor
{
	const char *Name;
	const struct DescriptorField *Fields;
	U32 ( *Tail )( struct DescriptorOutput *out, const U8 *data, U32 length, U32 offset );
};

enum ClassDescriptorType
{
	DESC_CLASS_INTERFACE = 0x24,
	DESC_CLASS_ENDPOINT = 0x25
};

#define DESC_BYTE(name)    { #name, 1, DESC_DECIMAL }
#define DESC_WORD(name)    { #name, 2, DESC_DECIMAL }
#define DESC_LONG(name)    { #name, 4, DESC_DECIMAL }
#define DESC_BITS(name, n) { #name, n, DESC_HEX }
#define DESC_VERSION(name) { #name, 2, DESC_BCD }
#define DESC_UUID(name)    { #name, 16, DESC_GUID }
#define DESC_FIELDS(...)   ( (const struct DescriptorField[]) { __VA_ARGS__, { NULL, 0, DESC_HEX } } )

const struct ClassDescriptor *Audio_Descriptor( const U8 *interface, const U8 *data, U32 length );
const struct ClassDescriptor *Video_Descriptor( const U8 *interface, const U8 *data, U32 length );
const struct ClassDescriptor *Class_Descriptor( const U8 *interface, const U8 *data, U32 length );

U32 Descriptor_Fields( struct DescriptorOutput *out, const U8 *data, U32 length, U32 offset,
	const struct DescriptorField *fields );
U32 Descriptor_List( struct DescriptorOutput *out, const U8 *data, U32 length, U32 offset,
	const struct DescriptorField *field, U32 count );
const struct ClassDescriptor *Descriptor_Lookup( const struct ClassDescriptor *table, U32 count, U32 index );

#define DESC_LOOKUP(table, index) Descriptor_Lookup( table, sizeof( table ) / sizeof( (table)[0] ), index )

#endif
