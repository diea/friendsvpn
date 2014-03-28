/*
 * XmlRPCParser.h
 *
 * Copyright (c) 2007 Sebastian Wiedenroth <wiedi@frubar.net>
 *                and Karl Glatz
 * 
 *
 * This class will parse an XML RPC request directly from a string.
 */

#ifndef XmlRPCParser_H
#define XmlRPCParser_H

#include <QtCore>
#include <QtXml>
#include <QtNetwork>
#include <QTextStream>
#include "maiaFault.h"
#include "qHttpRequest.h"

class XmlRPCParser : public QObject {
	Q_OBJECT
	
	public:
		XmlRPCParser(QObject *parent = 0);
		~XmlRPCParser();
		
		void readFromString(QString xmlrpc);

	signals:
		void getMethod(QString method, QObject **responseObject, const char **responseSlot);

	private slots:
	
	private:
		void parseCall(QString call);
		void sendResponse(QString content);

		bool invokeMethodWithVariants(QObject *obj,
		        const QByteArray &method, const QVariantList &args,
		        QVariant *ret, Qt::ConnectionType type = Qt::AutoConnection);
		static QByteArray getReturnType(const QMetaObject *obj,
			        const QByteArray &method, const QList<QByteArray> argTypes);
		

		QTextStream* textStream;
		QString headerString;
		QHttpRequestHeader *header;
		
};

#endif
