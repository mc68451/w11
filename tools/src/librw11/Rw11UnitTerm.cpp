// $Id: Rw11UnitTerm.cpp 1186 2019-07-12 17:49:59Z mueller $
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2013-2019 by Walter F.J. Mueller <W.F.J.Mueller@gsi.de>
// 
// Revision History: 
// Date         Rev Version  Comment
// 2019-05-18  1150   1.2    add detailed stats and StatInc{Rx,Tx}
// 2018-12-19  1090   1.1.7  use RosPrintf(bool)
// 2018-12-17  1085   1.1.6  use std::lock_guard instead of boost
// 2018-12-14  1081   1.1.5  use std::bind instead of boost
// 2018-12-09  1080   1.1.4  use HasVirt(); Virt() returns ref
// 2018-12-01  1076   1.1.3  use unique_ptr
// 2017-04-07   868   1.1.2  Dump(): add detail arg
// 2017-02-25   855   1.1.1  RcvNext() --> RcvQueueNext(); WakeupCntl() now pure
// 2013-05-03   515   1.1    use AttachDone(),DetachCleanup(),DetachDone()
// 2013-04-13   504   1.0    Initial version
// 2013-02-19   490   0.1    First draft
// ---------------------------------------------------------------------------

/*!
  \brief   Implemenation of Rw11UnitTerm.
*/

#include <functional>

#include "librtools/RparseUrl.hpp"
#include "librtools/RosFill.hpp"
#include "librtools/RosPrintf.hpp"
#include "librtools/Rexception.hpp"

#include "Rw11UnitTerm.hpp"

using namespace std;
using namespace std::placeholders;

/*!
  \class Retro::Rw11UnitTerm
  \brief FIXME_docs
*/

// all method definitions in namespace Retro
namespace Retro {

//------------------------------------------+-----------------------------------
//! Constructor

Rw11UnitTerm::Rw11UnitTerm(Rw11Cntl* pcntl, size_t index)
  : Rw11UnitVirt<Rw11VirtTerm>(pcntl, index),
    fTo7bit(false),
    fToEnpc(false),
    fTi7bit(false),
    fRcvQueue(),
    fLogFname(),
    fLogStream(),
    fLogOptCrlf(false),
    fLogCrPend(false),
    fLogLfLast(false)
{
  fStats.Define(kStatNPreAttDrop,    "NPreAttDrop",
                "snd bytes dropped prior attach");
  fStats.Define(kStatNRxFerr,  "NRxFerr",  "rx frame error");
  fStats.Define(kStatNRxChar,  "NRxChar",  "rx char (no ferr)");
  fStats.Define(kStatNRxNull,  "NRxNull",  "rx null char");
  fStats.Define(kStatNRx8bit,  "NRx8bit",  "rx with bit 8 set");
  fStats.Define(kStatNRxLine,  "NRxline",  "rx lines (CR)");
  fStats.Define(kStatNTxFerr,  "NTxFerr",  "tx frame error");
  fStats.Define(kStatNTxChar,  "NTxChar",  "tx char (no ferr)");
  fStats.Define(kStatNTxNull,  "NTxNull",  "tx null char");
  fStats.Define(kStatNTx8bit,  "NTx8bit",  "tx with bit 8 set");
  fStats.Define(kStatNTxLine,  "NTxline",  "tx lines (LF)");
}

//------------------------------------------+-----------------------------------
//! Destructor

Rw11UnitTerm::~Rw11UnitTerm()
{}

//------------------------------------------+-----------------------------------
//! FIXME_docs

const std::string& Rw11UnitTerm::ChannelId() const
{
  if (HasVirt()) return Virt().ChannelId();
  static string nil;
  return nil;
}  

//------------------------------------------+-----------------------------------
//! FIXME_docs

void Rw11UnitTerm::SetLog(const std::string& fname)
{
  if (fLogStream.is_open()) {
    if (fLogCrPend) fLogStream << "\r";
    fLogCrPend = false;
    fLogStream.close();
  }
  
  fLogFname.clear();
  if (fname.length() == 0) return;

  RparseUrl purl;
  RerrMsg emsg;
  if (!purl.Set(fname, "|app|bck=|crlf|", emsg)) 
    throw Rexception(emsg);
  if (!Rtools::CreateBackupFile(purl, emsg))
    throw Rexception(emsg);

  ios_base::openmode mode = ios_base::out;
  if (purl.FindOpt("app")) mode |= ios_base::app;

  fLogStream.open(purl.Path(), mode);
  if (!fLogStream.is_open()) {
    throw Rexception("Rw11UnitTerm::SetLog",
                     string("failed to open '")+purl.Path()+"'");
  }

  fLogFname = fname;
  fLogOptCrlf = purl.FindOpt("crlf");
  fLogCrPend = false;
  fLogLfLast = false;

  return;
}  

//------------------------------------------+-----------------------------------
//! FIXME_docs

void Rw11UnitTerm::StatIncRx(uint8_t ichr, bool ferr) 
{
  if (ferr) {
    fStats.Inc(kStatNRxFerr);
  } else {
    fStats.Inc(kStatNRxChar);
    if (ichr == 0)    fStats.Inc(kStatNRxNull);
    if (ichr & 0x80)  fStats.Inc(kStatNRx8bit);
    if (fTi7bit)      ichr &= 0x80;
    if (ichr == '\r') fStats.Inc(kStatNRxLine);   // count CR 
  }
  return;
}
  
//------------------------------------------+-----------------------------------
//! FIXME_docs

void Rw11UnitTerm::StatIncTx(uint8_t ochr, bool ferr) 
{
  if (ferr) {
    fStats.Inc(kStatNTxFerr);
  } else {
    fStats.Inc(kStatNTxChar);
    if (ochr == 0)    fStats.Inc(kStatNTxNull);
    if (ochr & 0x80)  fStats.Inc(kStatNTx8bit);
    if (fTo7bit)      ochr &= 0x80;
    if (ochr == '\n') fStats.Inc(kStatNTxLine);   // count LF
  }
  return;
}
  
//------------------------------------------+-----------------------------------
//! FIXME_docs

uint8_t Rw11UnitTerm::RcvQueueNext()
{
  if (RcvQueueEmpty()) return 0;
  uint8_t ochr = fRcvQueue.front();
  fRcvQueue.pop_front();
  return ochr;
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

size_t Rw11UnitTerm::Rcv(uint8_t* buf, size_t count)
{
  uint8_t* p = buf;
  for (size_t i=0; i<count && !fRcvQueue.empty(); i++) {
    *p++ = fRcvQueue.front();
    fRcvQueue.pop_front();
  }
  return p - buf;
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

bool Rw11UnitTerm::Snd(const uint8_t* buf, size_t count)
{
  bool ok = true;
  vector<uint8_t> bufmod;
  const uint8_t* bufout = buf;
  size_t bufcnt = count;

  if (fTo7bit || fToEnpc) {
    for (size_t i=0; i<count; i++) {
      uint8_t ochr = buf[i];
      if (fTo7bit) ochr &= 0177;
      if (fToEnpc) {
        if ((ochr>=040 && ochr<177) ||
             ochr=='\t' || ochr=='\n' || ochr=='\r') {
          bufmod.push_back(ochr);
        } else {
          if (ochr != 0) {
            bufmod.push_back('<');
            bufmod.push_back('0' + ((ochr>>6)&07) );
            bufmod.push_back('0' + ((ochr>>3)&07) );
            bufmod.push_back('0' +  (ochr    &07) );
            bufmod.push_back('>');
          }
        }
        
      } else {
        bufmod.push_back(ochr);
      }
    }
    bufout = bufmod.data();
    bufcnt = bufmod.size();
  }

  if (fLogStream.is_open()) {
    for (size_t i=0; i<bufcnt; i++) {
      uint8_t ochr = bufout[i];
      // the purpose of the 'crlf' filter is to map
      //   \r\n   -> \n
      //   \r\r\n -> \n  (any number of \r)
      //   \n\r   -> \n
      //   \n\r\r -> \n  (any number of \r)
      // and to ignore \0 chars
      if (fLogOptCrlf) {                    // crlf filtering on
        if (ochr == 0) continue;              // ignore \0 chars
        if (fLogCrPend) {
          if (ochr == '\r') continue;         // collapes multiple \r
          if (ochr != '\n') fLogStream << '\r'; // log \r if not followed by \n
          fLogCrPend = false;
        }
        if (ochr == '\r') {                   // \r seen 
          fLogCrPend = !fLogLfLast;           // remember \r if last wasn't \n 
          continue;
        }
      }
      fLogStream << char(ochr);
      fLogLfLast = (ochr == '\n');
    }
  }

  if (HasVirt()) {                          // if virtual device attached
    RerrMsg emsg;
    ok = Virt().Snd(bufout, bufcnt, emsg);
    // FIXME_code: handler errors
    
  } else {                                  // no virtual device attached
    if (Name() == "tta0") {                 // is it main console ?
      for (size_t i=0; i<bufcnt; i++) {       // than print to stdout 
        cout << char(bufout[i]) << flush;
      }
    } else {                                // otherwise discard
      fStats.Inc(kStatNPreAttDrop, double(bufcnt));  // and count at least...
    }
  }
  return ok;
}


//------------------------------------------+-----------------------------------
//! FIXME_docs

bool Rw11UnitTerm::RcvCallback(const uint8_t* buf, size_t count)
{
  // lock connect to protect rxqueue
  lock_guard<RlinkConnect> lock(Connect());

  bool que_empty_old = fRcvQueue.empty();
  for (size_t i=0; i<count; i++) {
    uint8_t ichr = buf[i];
    if (fTi7bit) ichr &= 0177;
    fRcvQueue.push_back(ichr);
  }
  bool que_empty_new = fRcvQueue.empty();
  if (que_empty_old && !que_empty_new) WakeupCntl();
  return true;
}

//------------------------------------------+-----------------------------------
//! FIXME_docs

void Rw11UnitTerm::Dump(std::ostream& os, int ind, const char* text,
                        int detail) const
{
  RosFill bl(ind);
  os << bl << (text?text:"--") << "Rw11UnitTerm @ " << this << endl;

  os << bl << "  fTo7bit:         " << RosPrintf(fTo7bit) << endl;
  os << bl << "  fToEnpc:         " << RosPrintf(fToEnpc) << endl;
  os << bl << "  fTi7bit:         " << RosPrintf(fTi7bit) << endl;
  {
    lock_guard<RlinkConnect> lock(Connect());
    size_t size = fRcvQueue.size();
    os << bl << "  fRcvQueue.size:  " << fRcvQueue.size() << endl;
    if (size > 0) {
      os << bl << "  fRcvQueue:       \"";
      size_t ocount = 0;
      for (size_t i=0; i<size; i++) {
        if (ocount >= 50) {
          os << "...";
          break;
        }
        uint8_t byt = fRcvQueue[i];
        if (byt >= 040 && byt <= 0176) {
          os << char(byt);
          ocount += 1;
        } else {
          os << "<" << RosPrintf(byt,"o0",3) << ">";
          ocount += 5;
        }
      }
      os << "\"" << endl;
    }
  }
  
  os << bl << "  fLogFname:       " << fLogFname << endl;
  os << bl << "  fLogStream.is_open: " << RosPrintf(fLogStream.is_open())<<endl;
  os << bl << "  fLogOptCrlf:     " << RosPrintf(fLogOptCrlf) << endl;
  os << bl << "  fLogCrPend:      " << RosPrintf(fLogCrPend) << endl;
  os << bl << "  fLogLfLast:      " << RosPrintf(fLogLfLast) << endl;

  Rw11UnitVirt<Rw11VirtTerm>::Dump(os, ind, " ^", detail);
  return;
} 

//------------------------------------------+-----------------------------------
//! FIXME_docs

void Rw11UnitTerm::AttachDone()
{
  Virt().SetupRcvCallback(std::bind(&Rw11UnitTerm::RcvCallback, this, _1, _2));
  return;
}


} // end namespace Retro
