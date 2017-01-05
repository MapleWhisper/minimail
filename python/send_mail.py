from email.header import Header
from email.mime.text import MIMEText
from email.utils import parseaddr, formataddr
import smtplib

import time

msg = MIMEText('lalal', 'plain', 'utf-8')

from_addr = 'hello@nihaoma'
pwd = '1234'
to_addr = '18111266760@163.com'
smtp_server = 'localhost'


def _format_addr(s):
    name, addr = parseaddr(s)
    return formataddr((Header(name, 'utf-8').encode(), addr))

msg['Date'] = time.ctime()

msg['From'] = _format_addr('PythonSender <%s>' % from_addr)
msg['To'] = _format_addr('<%s>' % to_addr)

msg['Subject'] = Header('Python Send Email', 'utf-8').encode()


server = smtplib.SMTP(smtp_server, 7725)

server.set_debuglevel(1)
server.login(from_addr, pwd)
server.sendmail(from_addr, [to_addr], msg.as_string())
server.quit()
