/*
 * libMaia - XmlRPCParser.cpp (inspired by MaiaXmlRpcServerConnection.hpp)
 */

#include "xmlRPCParser.h"
#include "xmlRPCTextServer.h"

XmlRPCParser::XmlRPCParser(QObject* parent) : QObject(parent) {
	header = NULL;
	textStream = NULL;
}

XmlRPCParser::~XmlRPCParser() {
	delete header;
	delete textStream;
}

void XmlRPCParser::readFromString(QString xmlrpc) {
	textStream = new QTextStream(&xmlrpc);
	QString lastLine = textStream->readLine();
	while((!lastLine.isNull()) && !header) {
		headerString += lastLine + "\r\n";
		qDebug() << lastLine;
		if(lastLine == "") { // XXX it was but readLine removes it "\r\n") { /* http header end */
			header = new QHttpRequestHeader(headerString);
			if(!header->isValid()) {
				/* return http error */
				qDebug() << "Invalid Header";
				return;
			} else if(header->method() != "POST") {
				/* return http error */
				qDebug() << "No Post!";
				return;
			} else if(!header->contentLength()) {
				/* return fault */
				qDebug() << "No Content Length";
				return;
			}
			break;
		}
		lastLine = textStream->readLine();
	}

	/* Put the \r\n */
	QString leftOver;
	while (!lastLine.isNull()) {
		lastLine = textStream->readLine();
		leftOver += lastLine + "\r\n";
	}

	if (header) {
		/* all data complete */
		parseCall(leftOver);
    }
}

void XmlRPCParser::sendResponse(QString content) {
	// XXX Use this if response needed, wait & see.
}

void XmlRPCParser::parseCall(QString call) {
	QDomDocument doc;
	QList<QVariant> args;
	QVariant ret;
	QString response;
	QObject *responseObject;
	const char *responseSlot;

	if(!doc.setContent(call)) { /* recieved invalid xml */
		MaiaFault fault(-32700, "parse error: not well formed");
		sendResponse(fault.toString());
		return;
	}
	
	QDomElement methodNameElement = doc.documentElement().firstChildElement("methodName");
	QDomElement params = doc.documentElement().firstChildElement("params");
	if(methodNameElement.isNull()) { /* invalid call */
		MaiaFault fault(-32600, "server error: invalid xml-rpc. not conforming to spec");
		sendResponse(fault.toString());
		return;
	}
	
	QString methodName = methodNameElement.text();
	
	emit getMethod(methodName, &responseObject, &responseSlot);
	if(!responseObject) { /* unknown method */
		MaiaFault fault(-32601, "server error: requested method not found");
		sendResponse(fault.toString());
		return;
	}
	
	QDomNode paramNode = params.firstChild();
	while(!paramNode.isNull()) {
		args << MaiaObject::fromXml( paramNode.firstChild().toElement());
		paramNode = paramNode.nextSibling();
	}
	
	if(!invokeMethodWithVariants(responseObject, responseSlot, args, &ret)) { /* error invoking... */
		MaiaFault fault(-32602, "server error: invalid method parameters");
		sendResponse(fault.toString());
		return;
	}
	
	
	if(ret.canConvert<MaiaFault>()) {
		response = ret.value<MaiaFault>().toString();
	} else {
		response = MaiaObject::prepareResponse(ret);
	}
	
	sendResponse(response);
}


/*	taken from http://delta.affinix.com/2006/08/14/invokemethodwithvariants/
	thanks to Justin Karneges once again :) */
bool XmlRPCParser::invokeMethodWithVariants(QObject *obj,
			const QByteArray &method, const QVariantList &args,
			QVariant *ret, Qt::ConnectionType type) {

	// QMetaObject::invokeMethod() has a 10 argument maximum
	if(args.count() > 10)
		return false;

	QList<QByteArray> argTypes;
	for(int n = 0; n < args.count(); ++n)
		argTypes += args[n].typeName();

	// get return type
	int metatype = 0;
	QByteArray retTypeName = getReturnType(obj->metaObject(), method, argTypes);
	if(!retTypeName.isEmpty()  && retTypeName != "QVariant") {
		metatype = QMetaType::type(retTypeName.data());
		if(metatype == 0) // lookup failed
			return false;
	}

	QGenericArgument arg[10];
	for(int n = 0; n < args.count(); ++n)
		arg[n] = QGenericArgument(args[n].typeName(), args[n].constData());

	QGenericReturnArgument retarg;
	QVariant retval;
	if(metatype != 0) {
		retval = QVariant(metatype, (const void *)0);
		retarg = QGenericReturnArgument(retval.typeName(), retval.data());
	} else { /* QVariant */
		retarg = QGenericReturnArgument("QVariant", &retval);
	}

	if(retTypeName.isEmpty()) { /* void */
		if(!QMetaObject::invokeMethod(obj, method.data(), type,
						arg[0], arg[1], arg[2], arg[3], arg[4],
						arg[5], arg[6], arg[7], arg[8], arg[9]))
			return false;
	} else {
		if(!QMetaObject::invokeMethod(obj, method.data(), type, retarg,
						arg[0], arg[1], arg[2], arg[3], arg[4],
						arg[5], arg[6], arg[7], arg[8], arg[9]))
			return false;
	}

	if(retval.isValid() && ret)
		*ret = retval;
	return true;
}

QByteArray XmlRPCParser::getReturnType(const QMetaObject *obj,
			const QByteArray &method, const QList<QByteArray> argTypes) {
	for(int n = 0; n < obj->methodCount(); ++n) {
		QMetaMethod m = obj->method(n);
#if QT_VERSION >= 0x050000
		QByteArray sig = m.methodSignature();
#else
		QByteArray sig = m.signature();
#endif
		int offset = sig.indexOf('(');
		if(offset == -1)
			continue;
		QByteArray name = sig.mid(0, offset);
		if(name != method)
			continue;
		if(m.parameterTypes() != argTypes)
			continue;

		return m.typeName();
	}
	return QByteArray();
}


